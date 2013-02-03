//===------- SemaTemplateVariadic.cpp - C++ Variadic Templates ------------===/
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//===----------------------------------------------------------------------===/
//
//  This file implements semantic analysis for C++0x variadic templates.
//===----------------------------------------------------------------------===/

#include "clang/Sema/Sema.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/ParsedTemplate.h"
#include "clang/Sema/ScopeInfo.h"
#include "clang/Sema/SemaInternal.h"
#include "clang/Sema/Template.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/TypeLoc.h"

using namespace clang;

//----------------------------------------------------------------------------
// Visitor that collects unexpanded parameter packs
//----------------------------------------------------------------------------

namespace {
  /// \brief A class that collects unexpanded parameter packs.
  class CollectUnexpandedParameterPacksVisitor :
    public RecursiveASTVisitor<CollectUnexpandedParameterPacksVisitor> 
  {
    typedef RecursiveASTVisitor<CollectUnexpandedParameterPacksVisitor>
      inherited;

    SmallVectorImpl<UnexpandedParameterPack> &Unexpanded;

    bool InLambda;
    

    // In certain situations, such as in nested generic lambdas
    // with variadic template parameters, we can end up with
    // expressions within an expansion pattern, that only contain
    // substituted packs that could not be expanded initially
    // because of a nested parameter pack that had not been 
    // substituted. When this flag is set to true, we collect
    // All substituted but unexpanded packs, which essentially
    // includes:
    //   FunctionParmPackExpr
    //   SubstNonTypeTemplateParmPackExpr
    // Consider the following code:
    //  auto L = [=]<class ... OTs>(OTs ... oargs)      <-- 1
    //              [=]<class ... ITs>(ITs ... iargs)   <-- 2
    //                variadic_print([=]() mutable {    <-- 3
    //                      variadic_print(oargs);      <-- A
    //                      variadic_print(iargs);
    //                      return 1;
    //                    }()...);
    //  auto M = L('A', 'B');
    //  When Instantiating 'L', and transforming Lambda '2' 
    //  and thus transforming Lambda '3', when 
    //  ActOnFullExpr calls DiagnoseUnexpandedParameterPack on 'oargs' 
    //   'oargs' returns true to containsUnexpandedParameterPack()
    //   but since it has been converted to a FunctionParmPackExpr
    //   by TransformDeclRefExpr (since 'oargs' can be expanded, but
    //   Lambda '3' can not be expanded until 'iargs' has been instantiated)
    //   the unexpandedParmPack Visitor Collector does not collect it.
    //   This is because, we do not normally collect the following as unexpanded
    //     - FunctionParmPackExpr
    //     - SubstNonTypeTemplatePackExpr (if OTs was int ... ONs) 
    bool CollectUnexpandableSubstitutedParameterPacks;
  public:
    explicit CollectUnexpandedParameterPacksVisitor(
                  SmallVectorImpl<UnexpandedParameterPack> &Unexpanded,
                  bool CollectUnexpandableSubstitutedParameterPacks = false)
      : Unexpanded(Unexpanded), InLambda(false),
        CollectUnexpandableSubstitutedParameterPacks(
                 CollectUnexpandableSubstitutedParameterPacks) { }
               
    bool shouldWalkTypesOfTypeLocs() const { return false; }
    
    //------------------------------------------------------------------------
    // Recording occurrences of (unexpanded) parameter packs.
    //------------------------------------------------------------------------

    /// \brief Record occurrences of template type parameter packs.
    bool VisitTemplateTypeParmTypeLoc(TemplateTypeParmTypeLoc TL) {
      if (TL.getTypePtr()->isParameterPack())
        Unexpanded.push_back(std::make_pair(TL.getTypePtr(), TL.getNameLoc()));
      return true;
    }

    /// \brief Record occurrences of template type parameter packs
    /// when we don't have proper source-location information for
    /// them.
    ///
    /// Ideally, this routine would never be used.
    bool VisitTemplateTypeParmType(TemplateTypeParmType *T) {
      if (T->isParameterPack())
        Unexpanded.push_back(std::make_pair(T, SourceLocation()));

      return true;
    }
    // Represents a reference to a function parameter pack that has been
    // substituted but not yet expanded.
    //
    // template<typename...Ts> struct S {
    //   template<typename...Us> auto f(Ts ...ts) -> decltype(g(Us(ts)...));
    // };
    // template struct S<int, int>;
    // In f(Ts ... ts) : ts is transformed into a FunctionParmPackExpr
    //   by TransformDeclRefExpr 
    bool VisitFunctionParmPackExpr(FunctionParmPackExpr *E) {

      if (!CollectUnexpandableSubstitutedParameterPacks) return true;
      ParmVarDecl *D = E->getParameterPack();
      Unexpanded.push_back(std::make_pair(D, E->getParameterPackLocation()));
      return true;
    }

    // Represents a reference to a non-type template parameter
    // that has been substituted with a template argument.
    // When a pack expansion in the source code contains multiple parameter packs
    // and those parameter packs correspond to different levels of template
    // parameter lists, this node is used to represent a non-type template
    // parameter pack from an outer level, which has already had its argument pack
    // substituted but that still lives within a pack expansion that itself
    // could not be instantiated. When actually performing a substitution into
    // that pack expansion (e.g., when all template parameters have corresponding
    // arguments), this type will be replaced with the appropriate underlying
    // expression at the current pack substitution index.
    bool VisitSubstNonTypeTemplateParmPackExpr(
                    SubstNonTypeTemplateParmPackExpr *E) {
      
      if (!CollectUnexpandableSubstitutedParameterPacks) return true;
      NonTypeTemplateParmDecl *D = E->getParameterPack();
      Unexpanded.push_back(std::make_pair(D, E->getParameterPackLocation()));
      return true;
    }

    /// \brief Record occurrences of function and non-type template
    /// parameter packs in an expression.
    bool VisitDeclRefExpr(DeclRefExpr *E) {
      if (E->getDecl()->isParameterPack())
        Unexpanded.push_back(std::make_pair(E->getDecl(), E->getLocation()));
      
      return true;
    }
    
    /// \brief Record occurrences of template template parameter packs.
    bool TraverseTemplateName(TemplateName Template) {
      if (TemplateTemplateParmDecl *TTP 
            = dyn_cast_or_null<TemplateTemplateParmDecl>(
                                                  Template.getAsTemplateDecl()))
        if (TTP->isParameterPack())
          Unexpanded.push_back(std::make_pair(TTP, SourceLocation()));
      
      return inherited::TraverseTemplateName(Template);
    }

    /// \brief Suppress traversal into Objective-C container literal
    /// elements that are pack expansions.
    bool TraverseObjCDictionaryLiteral(ObjCDictionaryLiteral *E) {
      if (!E->containsUnexpandedParameterPack())
        return true;

      for (unsigned I = 0, N = E->getNumElements(); I != N; ++I) {
        ObjCDictionaryElement Element = E->getKeyValueElement(I);
        if (Element.isPackExpansion())
          continue;

        TraverseStmt(Element.Key);
        TraverseStmt(Element.Value);
      }
      return true;
    }
    //------------------------------------------------------------------------
    // Pruning the search for unexpanded parameter packs.
    //------------------------------------------------------------------------

    /// \brief Suppress traversal into statements and expressions that
    /// do not contain unexpanded parameter packs.
    bool TraverseStmt(Stmt *S) { 
      Expr *E = dyn_cast_or_null<Expr>(S);
      if ((E && E->containsUnexpandedParameterPack()) || InLambda)
        return inherited::TraverseStmt(S);

      return true;
    }

    /// \brief Suppress traversal into types that do not contain
    /// unexpanded parameter packs.
    bool TraverseType(QualType T) {
      if ((!T.isNull() && T->containsUnexpandedParameterPack()) || InLambda)
        return inherited::TraverseType(T);

      return true;
    }

    /// \brief Suppress traversel into types with location information
    /// that do not contain unexpanded parameter packs.
    bool TraverseTypeLoc(TypeLoc TL) {
      if ((!TL.getType().isNull() && 
           TL.getType()->containsUnexpandedParameterPack()) ||
          InLambda)
        return inherited::TraverseTypeLoc(TL);

      return true;
    }

    /// \brief Suppress traversal of non-parameter declarations, since
    /// they cannot contain unexpanded parameter packs.
    bool TraverseDecl(Decl *D) { 
      if ((D && isa<ParmVarDecl>(D)) || InLambda)
        return inherited::TraverseDecl(D);

      return true;
    }

    /// \brief Suppress traversal of template argument pack expansions.
    bool TraverseTemplateArgument(const TemplateArgument &Arg) {
      if (Arg.isPackExpansion())
        return true;

      return inherited::TraverseTemplateArgument(Arg);
    }

    /// \brief Suppress traversal of template argument pack expansions.
    bool TraverseTemplateArgumentLoc(const TemplateArgumentLoc &ArgLoc) {
      if (ArgLoc.getArgument().isPackExpansion())
        return true;
      
      return inherited::TraverseTemplateArgumentLoc(ArgLoc);
    }

    /// \brief Note whether we're traversing a lambda containing an unexpanded
    /// parameter pack. In this case, the unexpanded pack can occur anywhere,
    /// including all the places where we normally wouldn't look. Within a
    /// lambda, we don't propagate the 'contains unexpanded parameter pack' bit
    /// outside an expression.
    bool TraverseLambdaExpr(LambdaExpr *Lambda) {
      // The ContainsUnexpandedParameterPack bit on a lambda is always correct,
      // even if it's contained within another lambda.
      if (!Lambda->containsUnexpandedParameterPack())
        return true;

      bool WasInLambda = InLambda;
      InLambda = true;

      // If any capture names a function parameter pack, that pack is expanded
      // when the lambda is expanded.
      for (LambdaExpr::capture_iterator I = Lambda->capture_begin(),
                                        E = Lambda->capture_end(); I != E; ++I)
        if (VarDecl *VD = I->getCapturedVar())
          if (VD->isParameterPack())
            Unexpanded.push_back(std::make_pair(VD, I->getLocation()));

      inherited::TraverseLambdaExpr(Lambda);

      InLambda = WasInLambda;
      return true;
    }
  };
}

/// \brief Diagnose all of the unexpanded parameter packs in the given
/// vector.
bool
Sema::DiagnoseUnexpandedParameterPacks(SourceLocation Loc,
                                       UnexpandedParameterPackContext UPPC,
                                 ArrayRef<UnexpandedParameterPack> Unexpanded) {
  if (Unexpanded.empty())
    return false;

  // If we are within a lambda expression, that lambda contains an unexpanded
  // parameter pack, and we are done.
  // FIXME: Store 'Unexpanded' on the lambda so we don't need to recompute it
  // later.
  for (unsigned N = FunctionScopes.size(); N; --N) {
    if (sema::LambdaScopeInfo *LSI =
          dyn_cast<sema::LambdaScopeInfo>(FunctionScopes[N-1])) {
      LSI->ContainsUnexpandedParameterPack = true;
      return false;
    }
  }
  
  SmallVector<SourceLocation, 4> Locations;
  SmallVector<IdentifierInfo *, 4> Names;
  llvm::SmallPtrSet<IdentifierInfo *, 4> NamesKnown;

  for (unsigned I = 0, N = Unexpanded.size(); I != N; ++I) {
    IdentifierInfo *Name = 0;
    if (const TemplateTypeParmType *TTP
          = Unexpanded[I].first.dyn_cast<const TemplateTypeParmType *>())
      Name = TTP->getIdentifier();
    else
      Name = Unexpanded[I].first.get<NamedDecl *>()->getIdentifier();

    if (Name && NamesKnown.insert(Name))
      Names.push_back(Name);

    if (Unexpanded[I].second.isValid())
      Locations.push_back(Unexpanded[I].second);
  }

  DiagnosticBuilder DB
    = Names.size() == 0? Diag(Loc, diag::err_unexpanded_parameter_pack_0)
                           << (int)UPPC
    : Names.size() == 1? Diag(Loc, diag::err_unexpanded_parameter_pack_1)
                           << (int)UPPC << Names[0]
    : Names.size() == 2? Diag(Loc, diag::err_unexpanded_parameter_pack_2)
                           << (int)UPPC << Names[0] << Names[1]
    : Diag(Loc, diag::err_unexpanded_parameter_pack_3_or_more)
        << (int)UPPC << Names[0] << Names[1];

  for (unsigned I = 0, N = Locations.size(); I != N; ++I)
    DB << SourceRange(Locations[I]);
  return true;
}

bool Sema::DiagnoseUnexpandedParameterPack(SourceLocation Loc, 
                                           TypeSourceInfo *T,
                                         UnexpandedParameterPackContext UPPC) {
  // C++0x [temp.variadic]p5:
  //   An appearance of a name of a parameter pack that is not expanded is 
  //   ill-formed.
  if (!T->getType()->containsUnexpandedParameterPack())
    return false;

  SmallVector<UnexpandedParameterPack, 2> Unexpanded;
  CollectUnexpandedParameterPacksVisitor(Unexpanded).TraverseTypeLoc(
                                                              T->getTypeLoc());
  assert(!Unexpanded.empty() && "Unable to find unexpanded parameter packs");
  return DiagnoseUnexpandedParameterPacks(Loc, UPPC, Unexpanded);
}

bool Sema::DiagnoseUnexpandedParameterPack(Expr *E,
                                        UnexpandedParameterPackContext UPPC) {
  // C++0x [temp.variadic]p5:
  //   An appearance of a name of a parameter pack that is not expanded is 
  //   ill-formed.
  if (!E->containsUnexpandedParameterPack())
    return false;

  SmallVector<UnexpandedParameterPack, 2> Unexpanded;
  CollectUnexpandedParameterPacksVisitor(Unexpanded).TraverseStmt(E);

  // Consider the following code:
  //  auto L = [=]<class ... OTs>(OTs ... oargs)        <-- 1
  //              [=]<class ... ITs>(ITs ... iargs)     <-- 2
  //                variadic_print([=]() mutable {    <-- 3
  //                      variadic_print(oargs);        <-- A
  //                      variadic_print(iargs);
  //                      return 1;
  //                    }()...);
  //  auto M = L('A', 'B');
  //  When Instantiating 'L', and transforming Lambda '2' 
  //  and thus transforming Lambda '3', when 
  //  ActOnFullExpr calls DiagnoseUnexpandedParameterPack on 'oargs' 
  //   'oargs' returns true to containsUnexpandedParameterPack()
  //   but since it has been converted to a FunctionParmPackExpr
  //   by TransformDeclRefExpr (since 'oargs' can be expanded, but
  //   Lambda '3' can not be expanded until 'iargs' has been instantiated)
  //   the unexpandedParmPack Visitor Collector does not collect it.
  //   This is because, we do not normally collect the following as unexpanded
  //     - FunctionParmPackExpr
  //     - SubstNonTypeTemplatePackExpr (if OTs was int ... ONs) 
  //   So if we are in a lambda context, lets try and collect
  //   all substituted, but non-expanded packs, and if we do
  //   collect such packs, just return success, and move on...
  if (Unexpanded.empty()) {
    sema::LambdaScopeInfo *LSI = getCurLambda();
    //  assert(isTransformingLambda() 
    assert(LSI && "Unless transforming a lambda, we should "
      "find unexpanded parameter packs!");
    CollectUnexpandedParameterPacksVisitor(Unexpanded, true).TraverseStmt(E);
    if (Unexpanded.size())
      return false;  
  }
  assert(!Unexpanded.empty() && "Unable to find unexpanded parameter packs");
  return DiagnoseUnexpandedParameterPacks(E->getLocStart(), UPPC, Unexpanded);
}

bool Sema::DiagnoseUnexpandedParameterPack(const CXXScopeSpec &SS,
                                        UnexpandedParameterPackContext UPPC) {
  // C++0x [temp.variadic]p5:
  //   An appearance of a name of a parameter pack that is not expanded is 
  //   ill-formed.
  if (!SS.getScopeRep() || 
      !SS.getScopeRep()->containsUnexpandedParameterPack())
    return false;

  SmallVector<UnexpandedParameterPack, 2> Unexpanded;
  CollectUnexpandedParameterPacksVisitor(Unexpanded)
    .TraverseNestedNameSpecifier(SS.getScopeRep());
  assert(!Unexpanded.empty() && "Unable to find unexpanded parameter packs");
  return DiagnoseUnexpandedParameterPacks(SS.getRange().getBegin(),
                                          UPPC, Unexpanded);
}

bool Sema::DiagnoseUnexpandedParameterPack(const DeclarationNameInfo &NameInfo,
                                         UnexpandedParameterPackContext UPPC) {
  // C++0x [temp.variadic]p5:
  //   An appearance of a name of a parameter pack that is not expanded is 
  //   ill-formed.
  switch (NameInfo.getName().getNameKind()) {
  case DeclarationName::Identifier:
  case DeclarationName::ObjCZeroArgSelector:
  case DeclarationName::ObjCOneArgSelector:
  case DeclarationName::ObjCMultiArgSelector:
  case DeclarationName::CXXOperatorName:
  case DeclarationName::CXXLiteralOperatorName:
  case DeclarationName::CXXUsingDirective:
    return false;

  case DeclarationName::CXXConstructorName:
  case DeclarationName::CXXDestructorName:
  case DeclarationName::CXXConversionFunctionName:
    // FIXME: We shouldn't need this null check!
    if (TypeSourceInfo *TSInfo = NameInfo.getNamedTypeInfo())
      return DiagnoseUnexpandedParameterPack(NameInfo.getLoc(), TSInfo, UPPC);

    if (!NameInfo.getName().getCXXNameType()->containsUnexpandedParameterPack())
      return false;

    break;
  }

  SmallVector<UnexpandedParameterPack, 2> Unexpanded;
  CollectUnexpandedParameterPacksVisitor(Unexpanded)
    .TraverseType(NameInfo.getName().getCXXNameType());
  assert(!Unexpanded.empty() && "Unable to find unexpanded parameter packs");
  return DiagnoseUnexpandedParameterPacks(NameInfo.getLoc(), UPPC, Unexpanded);
}

bool Sema::DiagnoseUnexpandedParameterPack(SourceLocation Loc,
                                           TemplateName Template,
                                       UnexpandedParameterPackContext UPPC) {
  
  if (Template.isNull() || !Template.containsUnexpandedParameterPack())
    return false;

  SmallVector<UnexpandedParameterPack, 2> Unexpanded;
  CollectUnexpandedParameterPacksVisitor(Unexpanded)
    .TraverseTemplateName(Template);
  assert(!Unexpanded.empty() && "Unable to find unexpanded parameter packs");
  return DiagnoseUnexpandedParameterPacks(Loc, UPPC, Unexpanded);
}

bool Sema::DiagnoseUnexpandedParameterPack(TemplateArgumentLoc Arg,
                                         UnexpandedParameterPackContext UPPC) {
  if (Arg.getArgument().isNull() || 
      !Arg.getArgument().containsUnexpandedParameterPack())
    return false;
  
  SmallVector<UnexpandedParameterPack, 2> Unexpanded;
  CollectUnexpandedParameterPacksVisitor(Unexpanded)
    .TraverseTemplateArgumentLoc(Arg);
  assert(!Unexpanded.empty() && "Unable to find unexpanded parameter packs");
  return DiagnoseUnexpandedParameterPacks(Arg.getLocation(), UPPC, Unexpanded);
}

void Sema::collectUnexpandedParameterPacks(TemplateArgument Arg,
                   SmallVectorImpl<UnexpandedParameterPack> &Unexpanded) {
  CollectUnexpandedParameterPacksVisitor(Unexpanded)
    .TraverseTemplateArgument(Arg);
}

void Sema::collectUnexpandedParameterPacks(TemplateArgumentLoc Arg,
                   SmallVectorImpl<UnexpandedParameterPack> &Unexpanded) {
  CollectUnexpandedParameterPacksVisitor(Unexpanded)
    .TraverseTemplateArgumentLoc(Arg);
}

void Sema::collectUnexpandedParameterPacks(QualType T,
                   SmallVectorImpl<UnexpandedParameterPack> &Unexpanded) {
  CollectUnexpandedParameterPacksVisitor(Unexpanded).TraverseType(T);  
}  

void Sema::collectUnexpandedParameterPacks(TypeLoc TL,
                   SmallVectorImpl<UnexpandedParameterPack> &Unexpanded) {
  CollectUnexpandedParameterPacksVisitor(Unexpanded).TraverseTypeLoc(TL);  
}  

void Sema::collectUnexpandedParameterPacks(CXXScopeSpec &SS,
                                           SmallVectorImpl<UnexpandedParameterPack> &Unexpanded) {
  NestedNameSpecifier *Qualifier = SS.getScopeRep();
  if (!Qualifier)
    return;
  
  NestedNameSpecifierLoc QualifierLoc(Qualifier, SS.location_data());
  CollectUnexpandedParameterPacksVisitor(Unexpanded)
    .TraverseNestedNameSpecifierLoc(QualifierLoc);
}

void Sema::collectUnexpandedParameterPacks(const DeclarationNameInfo &NameInfo,
                         SmallVectorImpl<UnexpandedParameterPack> &Unexpanded) {
  CollectUnexpandedParameterPacksVisitor(Unexpanded)
    .TraverseDeclarationNameInfo(NameInfo);
}


ParsedTemplateArgument 
Sema::ActOnPackExpansion(const ParsedTemplateArgument &Arg,
                         SourceLocation EllipsisLoc) {
  if (Arg.isInvalid())
    return Arg;

  switch (Arg.getKind()) {
  case ParsedTemplateArgument::Type: {
    TypeResult Result = ActOnPackExpansion(Arg.getAsType(), EllipsisLoc);
    if (Result.isInvalid())
      return ParsedTemplateArgument();

    return ParsedTemplateArgument(Arg.getKind(), Result.get().getAsOpaquePtr(), 
                                  Arg.getLocation());
  }

  case ParsedTemplateArgument::NonType: {
    ExprResult Result = ActOnPackExpansion(Arg.getAsExpr(), EllipsisLoc);
    if (Result.isInvalid())
      return ParsedTemplateArgument();
    
    return ParsedTemplateArgument(Arg.getKind(), Result.get(), 
                                  Arg.getLocation());
  }
    
  case ParsedTemplateArgument::Template:
    if (!Arg.getAsTemplate().get().containsUnexpandedParameterPack()) {
      SourceRange R(Arg.getLocation());
      if (Arg.getScopeSpec().isValid())
        R.setBegin(Arg.getScopeSpec().getBeginLoc());
      Diag(EllipsisLoc, diag::err_pack_expansion_without_parameter_packs)
        << R;
      return ParsedTemplateArgument();
    }
      
    return Arg.getTemplatePackExpansion(EllipsisLoc);
  }
  llvm_unreachable("Unhandled template argument kind?");
}

TypeResult Sema::ActOnPackExpansion(ParsedType Type, 
                                    SourceLocation EllipsisLoc) {
  TypeSourceInfo *TSInfo;
  GetTypeFromParser(Type, &TSInfo);
  if (!TSInfo)
    return true;

  TypeSourceInfo *TSResult = CheckPackExpansion(TSInfo, EllipsisLoc,
                                                llvm::Optional<unsigned>());
  if (!TSResult)
    return true;
  
  return CreateParsedType(TSResult->getType(), TSResult);
}

TypeSourceInfo *Sema::CheckPackExpansion(TypeSourceInfo *Pattern,
                                         SourceLocation EllipsisLoc,
                                       llvm::Optional<unsigned> NumExpansions) {
  // Create the pack expansion type and source-location information.
  QualType Result = CheckPackExpansion(Pattern->getType(), 
                                       Pattern->getTypeLoc().getSourceRange(),
                                       EllipsisLoc, NumExpansions);
  if (Result.isNull())
    return 0;
  
  TypeSourceInfo *TSResult = Context.CreateTypeSourceInfo(Result);
  PackExpansionTypeLoc TL = cast<PackExpansionTypeLoc>(TSResult->getTypeLoc());
  TL.setEllipsisLoc(EllipsisLoc);
  
  // Copy over the source-location information from the type.
  memcpy(TL.getNextTypeLoc().getOpaqueData(),
         Pattern->getTypeLoc().getOpaqueData(),
         Pattern->getTypeLoc().getFullDataSize());
  return TSResult;
}

QualType Sema::CheckPackExpansion(QualType Pattern,
                                  SourceRange PatternRange,
                                  SourceLocation EllipsisLoc,
                                  llvm::Optional<unsigned> NumExpansions) {
  // C++0x [temp.variadic]p5:
  //   The pattern of a pack expansion shall name one or more
  //   parameter packs that are not expanded by a nested pack
  //   expansion.
  if (!Pattern->containsUnexpandedParameterPack()) {
    Diag(EllipsisLoc, diag::err_pack_expansion_without_parameter_packs)
      << PatternRange;
    return QualType();
  }

  return Context.getPackExpansionType(Pattern, NumExpansions);
}

ExprResult Sema::ActOnPackExpansion(Expr *Pattern, SourceLocation EllipsisLoc) {
  return CheckPackExpansion(Pattern, EllipsisLoc, llvm::Optional<unsigned>());
}

ExprResult Sema::CheckPackExpansion(Expr *Pattern, SourceLocation EllipsisLoc,
                                    llvm::Optional<unsigned> NumExpansions) {
  if (!Pattern)
    return ExprError();
  
  // C++0x [temp.variadic]p5:
  //   The pattern of a pack expansion shall name one or more
  //   parameter packs that are not expanded by a nested pack
  //   expansion.
  if (!Pattern->containsUnexpandedParameterPack()) {
    Diag(EllipsisLoc, diag::err_pack_expansion_without_parameter_packs)
    << Pattern->getSourceRange();
    return ExprError();
  }
  
  // Create the pack expansion expression and source-location information.
  return Owned(new (Context) PackExpansionExpr(Context.DependentTy, Pattern,
                                               EllipsisLoc, NumExpansions));
}

/// \brief Retrieve the depth and index of a parameter pack.
static std::pair<unsigned, unsigned> 
getDepthAndIndex(NamedDecl *ND) {
  if (TemplateTypeParmDecl *TTP = dyn_cast<TemplateTypeParmDecl>(ND))
    return std::make_pair(TTP->getDepth(), TTP->getIndex());
  
  if (NonTypeTemplateParmDecl *NTTP = dyn_cast<NonTypeTemplateParmDecl>(ND))
    return std::make_pair(NTTP->getDepth(), NTTP->getIndex());
  
  TemplateTemplateParmDecl *TTP = cast<TemplateTemplateParmDecl>(ND);
  return std::make_pair(TTP->getDepth(), TTP->getIndex());
}

// Check to see if a DeclarationPack is referred to by a 
// pack expansion within the expr
//  - Check if a lambda capture is a pack expansion
//    and it refers to the ParameterPack
//  - or within a PackExpansionExpr, check to see
//    if any contained DeclRefExprs refer to the
//    Parameter Pack
struct CheckIfDeclIsExpandedInExpr : 
  RecursiveASTVisitor<CheckIfDeclIsExpandedInExpr> {
private:
  typedef RecursiveASTVisitor<CheckIfDeclIsExpandedInExpr> 
    inherited;
  // A visitor that when passed in a Declaration during construction
  //   will set 'Yes' to true, if during Visitation of an expression
  //   the TheDecl is referred to.  
  struct IsDeclReferredTo : RecursiveASTVisitor<IsDeclReferredTo> {
  private:
    NamedDecl *PackDeclOfInterest;
    bool Yes;
  public:
    IsDeclReferredTo(NamedDecl *TheDecl) : 
        PackDeclOfInterest(TheDecl), Yes(false)
    { }

    bool VisitDeclRefExpr(DeclRefExpr* E) {
      if (E->getDecl() == PackDeclOfInterest)
        Yes = true;
      return true;
    }
    bool yes() const { return Yes; }
  };

  Sema    &SemaRef;
  // The Capture which we are interested in regarding whether it is expanded
  // in the body of the lambda
  NamedDecl *PackDeclOfInterest;   
  bool Yes;
  SourceLocation EllipsisLoc;
public:
  CheckIfDeclIsExpandedInExpr(Sema &S, NamedDecl *DeclOfInterest) : 
      SemaRef(S), PackDeclOfInterest(DeclOfInterest), Yes(false) {
  }
  // We need to check the lambda captures separately 
  // since they are not visited via VisitPackExpansionExpr
  // since pack expansions within explicit captures
  // are stored as isPackExpansion() == true
  bool TraverseLambdaCapture(LambdaExpr::Capture C) {
    if (C.capturesVariable() && 
          C.getCapturedVar() == PackDeclOfInterest) {
      if (!Yes)
        Yes = C.isPackExpansion();
    }
    return true;
  }
  // Check all PackExpansionExpr's by a nested visitor
  // for whether the DeclRefExpr refers to our
  // PackDeclOfInterest
  bool VisitPackExpansionExpr(PackExpansionExpr *E) {
    Expr *Pattern = E->getPattern();
    if (!Yes) { // if we have already been set, no need to go deeper
      IsDeclReferredTo DR(PackDeclOfInterest);
      DR.TraverseStmt(E);
      Yes = DR.yes();
      if (Yes)
        EllipsisLoc = E->getEllipsisLoc();
    }
    return true;
  }

  bool yes() const { return Yes; }
  SourceLocation getEllipsisLoc() const { return EllipsisLoc; }
};


// Visits an expression to check if anywhere within it 
// there is a place where the PackDecl is referred to and
// is NOT part of a pack expansion.
// This can be checked by visiting all DeclRefExprs that are NOT part of a
// PackExpansionExpr or a Capture that refers to the PackDecl but is not
// a (is)PackExpansion...
// I.E.
// 1) find a matching capture that is not pack expanded
// 2) find a decl that refers to the DeclOfInterest (since we
//     do not traverse PackExprs, any such decl can only
//     refer to a non expanded pack
struct CheckIfDeclIsNOTExpandedInExpr : 
  RecursiveASTVisitor<CheckIfDeclIsNOTExpandedInExpr> {
private:
  typedef RecursiveASTVisitor<CheckIfDeclIsNOTExpandedInExpr> 
    inherited;
 
  Sema    &SemaRef;
  // The Decl which was are interested in
  NamedDecl *PackDeclOfInterest;   
  bool Yes;
  SourceLocation EllipsisLoc;
public:
  CheckIfDeclIsNOTExpandedInExpr(Sema &S, NamedDecl *DeclOfInterest) : 
      SemaRef(S), PackDeclOfInterest(DeclOfInterest), Yes(false) {
  }
  bool VisitDeclRefExpr(DeclRefExpr *E) {
    if (E->getDecl() == PackDeclOfInterest)
      Yes = true;
    return true;
  }
  // If even one unexpanded capture refers to this Decl
  // we succeed.
  bool TraverseLambdaCapture(LambdaExpr::Capture C) {
    if (C.capturesVariable() && 
            C.getCapturedVar() == PackDeclOfInterest) {
        if (!C.isPackExpansion())
          Yes = true;
    }
    return true;
  }

  // We do NOT want to go deeper into a pack expansion expressions 
  // returning true, allows the visitations to continue
  // if we return false, then visitation of other nodes
  // also gets aborted.
  bool TraversePackExpansionExpr(PackExpansionExpr *E) {      
    return true;
  }

  bool yes() const { return Yes; }
};

namespace clang {
  // This returns true, if the Expression E contains references to the
  // PackDecl contained only within PackExpansionExpr's 
  // i.e. consider:

  // []<class ... Ts, int ... Ns>(type_int_pack<Ts, Ns> ... Args)
  //     variadic_fun( ([=]() {
  //            Ts t = Args; // --> is a non-expansion that refers to Args
  //            int i[]{ Ns ... };   // All references to Ns are expansions
  //            variadic_fun(Args ... ); // All references to Args are not 
  //                                     // all expansions
  //          })() ...);
  //  if PackDecl is 'Ns' above, will return true.             
  bool refersOnlyToNestedPackExpansions(Expr *E, 
    const MultiLevelTemplateArgumentList& TemplateArgs, Sema &S, 
    NamedDecl *PackDecl) {
      // First check to see if there is some expansion of this decl
      CheckIfDeclIsExpandedInExpr C(S, PackDecl);
      C.TraverseStmt(E);
      // Next check and makes sure there is no instance where 
      // the pack decl is referred to within 'E' in a non-expansion
      // context.
      CheckIfDeclIsNOTExpandedInExpr CNot(S, PackDecl);
      CNot.TraverseStmt(E);
      return C.yes() && !CNot.yes();
  }


}

// A visitor that checks if a decl refers to a nested
// lambda parameter pack ...
// 
//  InnerArgs below...
// []<int ... Js>() 
//   variadic_fun( []<class ... InnerTs>(InnerTs ... InnerArgs)
//     {
//       variadic_fun(InnerArgs...); //ok
//       int j = Js; //  ok 
//     } ...);
//
struct CheckIfPackDeclRefersToNestedLambdaParamPack : 
  RecursiveASTVisitor<CheckIfPackDeclRefersToNestedLambdaParamPack> {

    typedef RecursiveASTVisitor<CheckIfPackDeclRefersToNestedLambdaParamPack>
      inherited;

    // The Decl which was are interested in
    NamedDecl *PackDeclOfInterest;   
    bool Yes;

    Sema &SemaRef;
    const MultiLevelTemplateArgumentList& DeducedTemplateArgs;
    CheckIfPackDeclRefersToNestedLambdaParamPack(Sema& S, 
      const MultiLevelTemplateArgumentList &TemplateArgs,
      NamedDecl *PackDeclOfInterest) : 
    SemaRef(S), DeducedTemplateArgs(TemplateArgs), 
      PackDeclOfInterest(PackDeclOfInterest), Yes(false)
    { }

    // There is no VisitLambdaExpr, since Traverse breaks
    // a LambdaExpr into Visitable Parts - check
    // TraverseLambdaExpr in RecursiveASTVisitor
    // to see exactly what is visitable ...
    bool TraverseLambdaExpr(LambdaExpr* E) {

      // Iterate through all the parameters of this 
      // lambda and check if we have a parameter pack
      CXXMethodDecl *CallOp = E->getCallOperator();

      for (size_t I = 0; I < CallOp->getNumParams(); ++I)
      {
        ParmVarDecl *PVD = CallOp->getParamDecl(I);
        if (!Yes && PVD == PackDeclOfInterest)  {
          Yes = true;
        }
      }
      bool ret = inherited::TraverseLambdaExpr(E);
      return ret; 
    }

    bool yes() const { return Yes; }
};


struct CheckIfPackTypeRefersToNestedLambdaParamPack : 
  RecursiveASTVisitor<CheckIfPackTypeRefersToNestedLambdaParamPack> {

  typedef RecursiveASTVisitor<CheckIfPackTypeRefersToNestedLambdaParamPack>
    inherited;


  Sema &SemaRef;
  const MultiLevelTemplateArgumentList& DeducedTemplateArgs;
  bool Yes;
  const TemplateTypeParmType *const ThePackType;

  CheckIfPackTypeRefersToNestedLambdaParamPack(Sema& S, 
    const MultiLevelTemplateArgumentList &TemplateArgs, 
    const TemplateTypeParmType *ThePackType) : 
  SemaRef(S), DeducedTemplateArgs(TemplateArgs), Yes(false),
    ThePackType(ThePackType)
  { }
  
  bool TraverseLambdaExpr(LambdaExpr* E) { 
    // Iterate through all the template parameters of this 
    // lambda and check if we have a parameter pack
    TemplateParameterList *TPL = E->getTemplateParameterList();
    if (TPL) {
      for (size_t I = 0; I < TPL->size(); ++I) {
        const NamedDecl *ND = TPL->getParam(I);
        if (const TemplateTypeParmDecl *TTPD = 
                dyn_cast<const TemplateTypeParmDecl>(ND)) {
          const Type* Ty = TTPD->getTypeForDecl();
          if (Ty == ThePackType)
            Yes = true;
        }
      }
    }
      
    bool ret = inherited::TraverseLambdaExpr(E);
    return ret; 
  }

  bool yes() const { return Yes; }

};




namespace clang {
bool refersToNestedLambdaParameterPack(Expr *E, 
        const MultiLevelTemplateArgumentList& TemplateArgs, Sema &S, 
        ParmVarDecl *PackDecl) {

  CheckIfPackDeclRefersToNestedLambdaParamPack C(S, TemplateArgs, PackDecl);
  C.TraverseStmt(E);
  return C.yes(); // C.AllLambdaParamPacks[PackDecl];
}


bool refersToNestedLambdaParameterPackType(Expr *E, 
    const MultiLevelTemplateArgumentList& TemplateArgs, Sema &S, 
    const TemplateTypeParmType *PackType) {

  CheckIfPackTypeRefersToNestedLambdaParamPack C(S, TemplateArgs, PackType);
  C.TraverseStmt(E);
  return C.yes();

}

} // end namespace clang


// Check if the Expression contains a reference to the 
// PackDecl in an unexpandable context
// 
struct CheckIfPackDeclRefersToUnexpandableNestedLambdaParamPack : 
  RecursiveASTVisitor<
          CheckIfPackDeclRefersToUnexpandableNestedLambdaParamPack> {

    typedef RecursiveASTVisitor<CheckIfPackDeclRefersToUnexpandableNestedLambdaParamPack>
      inherited;

    // The Decl which was are interested in
    NamedDecl *PackDeclOfInterest;   
    bool Yes;

    Sema &SemaRef;
    const MultiLevelTemplateArgumentList& DeducedTemplateArgs;
    CheckIfPackDeclRefersToUnexpandableNestedLambdaParamPack(Sema& S, 
      const MultiLevelTemplateArgumentList &TemplateArgs,
      NamedDecl *PackDeclOfInterest) : 
    SemaRef(S), DeducedTemplateArgs(TemplateArgs), 
      PackDeclOfInterest(PackDeclOfInterest), Yes(false)
    { }

    bool TraverseLambdaExpr(LambdaExpr* E) {

      // Find the nested lambda whose pack it is
      CXXMethodDecl *CallOp = E->getCallOperator();

      for (size_t I = 0; I < CallOp->getNumParams(); ++I)
      {
        ParmVarDecl *PVD = CallOp->getParamDecl(I);
        if (PVD == PackDeclOfInterest && !Yes)  {
          // now check to see if we are referred to in the body of this lambda
          // in a non pack expansion context
          CheckIfDeclIsNOTExpandedInExpr CNot(SemaRef, PackDeclOfInterest);
          CNot.TraverseStmt(E->getBody());
          Yes = CNot.yes();
        }
      }
      bool ret = inherited::TraverseLambdaExpr(E);
      return true; 
    }

    bool yes() const { return Yes; }
};

namespace clang {
  //
  // []<int ... Js>() 
  //   variadic_fun( []<class ... InnerTs>(InnerTs ... InnerArgs)
  //     {
  //       variadic_fun(InnerArgs...); //ok
  //       int j = Js; //  ok
  //       return InnerArgs; // Not ok <-- this is unexpandable
  //     }(1, 2, 3) ...);
  //
  bool containsUnexpandableNestedLambdaParameterPack(Expr *E, 
    const MultiLevelTemplateArgumentList& TemplateArgs, Sema &S, 
    ParmVarDecl *NestedLambdaPackDecl) {
      if ( refersToNestedLambdaParameterPack(E, TemplateArgs, S, 
                                          NestedLambdaPackDecl) ) {
        CheckIfPackDeclRefersToUnexpandableNestedLambdaParamPack 
                                  C(S, TemplateArgs, NestedLambdaPackDecl);
        C.TraverseStmt(E);
        return C.yes(); 
      }
      return false;
  }

} // end namespace clang

// Defined in SemaExpr.cpp
bool IsLambdaCallOpDeclContext(const DeclContext* DC);

// CheckParameterPacksForExpansion:
// This checks to see if every unexpanded pack can be expanded
// - if even one pack can not be expanded, none of the packs can
//   be expanded.
//   The algorithm is as follows:
//     - if the UPP is a function parameter pack, check to see
//         if TemplateArgs have a deduction for it
//         and if so, do we have a local instantiation in the
//         CurrentInstantiationScope that is a sequence of substituted
//         declarations - if not, then we are not ready to expand this pack.
//     - otherwise we just check to see if the pack has been deduced
//       or not, and if not, don't expand.
//
//  It also checks to see whether the expansion pattern needs to be retained
bool Sema::CheckParameterPacksForExpansion(SourceLocation EllipsisLoc,
                                           SourceRange PatternRange,
                                   ArrayRef<UnexpandedParameterPack> Unexpanded,
                             const MultiLevelTemplateArgumentList &TemplateArgs,
                                           bool &ShouldExpand,
                                           bool &RetainExpansion,
                                     llvm::Optional<unsigned> &NumExpansions) {                                        
  ShouldExpand = true;
  RetainExpansion = false;
  std::pair<IdentifierInfo *, SourceLocation> FirstPack;
  bool HaveFirstPack = false;
  
  for (ArrayRef<UnexpandedParameterPack>::iterator i = Unexpanded.begin(),
                                                 end = Unexpanded.end();
                                                  i != end; ++i) {
    // Compute the depth and index for this parameter pack.
    unsigned Depth = 0, Index = 0;
    IdentifierInfo *Name;
    bool IsFunctionParameterPack = false;

    // If this is a function parameter pack, set the type
    // of the pack 
    const TemplateTypeParmType *ParamDeclTypeOfPack = 0;
    

    // If this is a template type param pack, the Type of this pack
    const TemplateTypeParmType *PackTemplateType = 0;
    
    // Or If this is a Decl, then set the Decl of this pack - 
    // can be a Decl of a NonType, Type and Template Decl Pack
    NamedDecl *PackDecl = 0;

    if (const TemplateTypeParmType *TTP
        = i->first.dyn_cast<const TemplateTypeParmType *>()) {
      Depth = TTP->getDepth();
      Index = TTP->getIndex();
      Name = TTP->getIdentifier();
      PackTemplateType = TTP;
    } else {
      NamedDecl *ND = i->first.get<NamedDecl *>();
      PackDecl = ND;
      if (isa<ParmVarDecl>(ND)) {
        ParmVarDecl *PD = cast<ParmVarDecl>(ND);
        QualType ParamType = PD->getType();
        assert(PD->isParameterPack() && "CheckParameterPacks passed a non Pack!");
        const PackExpansionType* PackType = 
                      dyn_cast<const PackExpansionType>(
                                              ParamType.getTypePtr());
        assert(PackType && 
              "The Parameter could not be converted to a PackExpansionType!");
        // Get the type of the pattern 
        QualType PatternType = PackType->getPattern();
        QualType BasePatternType = PatternType;
        // If this is a reference or pointer, get the base type
        if (PatternType->isReferenceType() || PatternType->isPointerType())
          BasePatternType = PatternType->getPointeeType(); 
        ParamDeclTypeOfPack = 
                  dyn_cast_or_null<const TemplateTypeParmType>(
                                              BasePatternType.getTypePtr()); 
        if (ParamDeclTypeOfPack) {
          Depth = ParamDeclTypeOfPack->getDepth();
          Index = ParamDeclTypeOfPack->getIndex();
        }
        IsFunctionParameterPack = true;


      }
      else
        llvm::tie(Depth, Index) = getDepthAndIndex(ND);        
      
      Name = ND->getIdentifier();
    }
    
    // Determine the size of this argument pack.
    unsigned NewPackSize;    

    // if this pack is a function parameter pack and the depth
    // of this pack is less than the depth of the deduced arguments
    // lets see if we can expand this pack...
    if (IsFunctionParameterPack && 
            (!ParamDeclTypeOfPack || (ParamDeclTypeOfPack->getDepth() < 
                                              TemplateArgs.getNumLevels()))) {
      
      // Figure out whether we're instantiating to an argument pack or not.
      typedef LocalInstantiationScope::DeclArgumentPack DeclArgumentPack;
      // We know PackDecl is castable to a ParmVarDecl
      ParmVarDecl *ParmDecl = cast<ParmVarDecl>(PackDecl);
       
      llvm::PointerUnion<Decl *, DeclArgumentPack *> *Instantiation
        = CurrentInstantiationScope->hasInstantiationOf(ParmDecl);
      
      if (!Instantiation) {
        
        // If we don't have an instantiation, check if we are instantiating a lambda
        // that has a cached expanded pack expansion
        typedef sema::LambdaScopeInfo LambdaScopeInfo;
        LambdaScopeInfo* LSI = getCurLambda();

        assert(LSI && "In CheckParameterPacksForExpansion if we can't map a "
          "a Decl to an instantiated Decl, we should be in a lambda context (i.e. "
          " getCurLambda() should be non-null!");

        typedef LambdaScopeInfo::CapturedPackMapTy::iterator PackMapItTy;
        // Get the cached packs that have been substituted, but we could 
        // not capture at the time of transformation of the lambda, 
        // because we did not know which element of the pack we would
        // need to capture ....
        // For e.g.
        // auto L = []<class ... OuterTs>(OuterTs ... OuterArgs)    <-- 1
        //            [=]<class ... InnerTs>(InnerTs ... InnerArgs) <-- 2
        //               [=]()                                      <-- 3
        //                  variadic_call(
        //                        [=]() mutable {                   <-- 4 
        //                                OuterArgs = InnerArgs; return 0; } 
        //                            ...);
        // auto M = L('A', 1);
        // auto N = M(2, 3.14);                                     <-- 5
        //   When transforming '3' at '5' there is no problem, since we 
        //   know we have to capture all the elements of the pack expansions
        //   in Lambda '3' -- but when transforming '4' we need to use 
        //   the cached substituted packs (InnerArgs, and OuterArgs) to
        //   capture specific elements of the pack within lambda '4'  
        LambdaScopeInfo::CapturedPackMapTy& PackMap = 
          LSI->getDeclPacksForLaterExpansionAndCapture();
        for (PackMapItTy It = PackMap.begin(), End = PackMap.end(); 
          It != End; ++It) {
            VarDecl *VD = It->first;
            SmallVectorImpl<Decl*>& PackDecl = It->second;
            assert(VD->isParameterPack());
            // Instantiate all the elements of the pack
            CurrentInstantiationScope->MakeInstantiatedLocalArgPack(VD);
            for (size_t I = 0; I < PackDecl.size(); ++I)
              CurrentInstantiationScope->InstantiatedLocalPackArg(VD, PackDecl[I]);

        }
        Instantiation = CurrentInstantiationScope->hasInstantiationOf(ParmDecl);
        assert(Instantiation && "There is still no instantiation for the FunctionParameter Pack "
         " Decl even after attempting to instantiate all the substituted cached packs!");

      }
      if (Instantiation->is<DeclArgumentPack *>()) {
        // We could expand this function parameter pack.
        NewPackSize = Instantiation->get<DeclArgumentPack *>()->size();
      } else {
        // We can't expand this function parameter pack, so we can't expand
        // the pack expansion.
        ShouldExpand = false;
        continue;
      }
    } else { //!IsFunctionParameterPack || DepthOfFPack > TemplateArgs.levels 
      // If we don't have a template argument at this depth/index, then we 
      // cannot expand the pack expansion. Make a note of this, but we still 
      // want to check any parameter packs we *do* have arguments for.
      if (Depth >= TemplateArgs.getNumLevels() ||
          !TemplateArgs.hasTemplateArgument(Depth, Index)) {
        ShouldExpand = false;
        continue;
      }
      
      // Determine the size of the argument pack.
      NewPackSize = TemplateArgs(Depth, Index).pack_size();
    }
    
    // C++0x [temp.arg.explicit]p9:
    //   Template argument deduction can extend the sequence of template 
    //   arguments corresponding to a template parameter pack, even when the
    //   sequence contains explicitly specified template arguments.
    if (!IsFunctionParameterPack) {
      if (NamedDecl *PartialPack 
                    = CurrentInstantiationScope->getPartiallySubstitutedPack()){
        // We would only want to retain the expansion, if the PartialPack is in the
        // same decl-context as the pack we are processing
        // i.e. auto L =  []<int ... Ns, class ... Ts>(Ts ... args)
        //                  [=]()
        //                   [=](typename get_type<Ns, Ts>::type ... args2)
        // L.operator<1, 2>('A', 3.14);
        //  We can find the partial pack, but it is in a different decl-context
        //  so we can not expand it via this deduction, so do not retain it...
        // LambdaExpr *E = isTransformingLambda();
        //   get the current context
        DeclContext *CurrentCtx = CurContext;
        if (IsLambdaCallOpDeclContext(CurrentCtx))
          CurrentCtx = CurrentCtx->getParent()->getParent();

        bool SkipRetainExpansion = false;

        // Get the ActiveInstantiation Info ...
        ActiveTemplateInstantiation& Inst = 
                    ActiveTemplateInstantiations.back();

        // If we are just doing deduced argument substitution, do NOT
        // retain any expansion pattern i.e.
        // Consider :
        // struct F {
        //   template<int ... Ns> void foo(int_pack<Ns ...> ip);
        // };
        // F().foo<1, 2>(int_pack<1, 2, 3> ip);
        // When substituting <1, 2, 3> during FinalizingArgumentDeduction
        // Do Not retain the pack expansion 'Ns ...' as int_pack<1, 2, 3, Ns ...>
        // Because we will not be deducing any further, we are done with
        // deduction and we are just substituting.
        bool IsSubstitutingDeducedArguments = Inst.Kind == 
          ActiveTemplateInstantiation::DeducedTemplateArgumentSubstitution;
        SkipRetainExpansion = IsSubstitutingDeducedArguments;
        
        if (!SkipRetainExpansion) {
          // if the pack expansion belongs to a nested lambda, 
          // and the pack expansion refers to a template parameter pack 
          // from an outer lambda, we do not need to retain the expansion pattern
          
          // Get The DeclContext of the partial pack
          //  i.e. auto L = []<class ... OuterTs>(OuterTs ... OuterArgs)
          //                   []<class ... InnerTs>( 
          DeclContext* PartialPackDC = PartialPack->getDeclContext();
          DeclContext* PartialPackDCParent = PartialPackDC->getParent();
          CXXRecordDecl* PLambdaClass = dyn_cast_or_null<CXXRecordDecl>(PartialPackDCParent);
          if (PLambdaClass && !PLambdaClass->isLambda())
            PLambdaClass = 0;
          // FVTODO: this should also be made to work with function templates, not just
          // lambda call ops
          while (CurrentCtx && PLambdaClass && 
                              IsLambdaCallOpDeclContext(CurrentCtx)) {
         
            CXXRecordDecl *CurLambdaClass = dyn_cast<CXXRecordDecl>(
                                                    CurrentCtx->getParent());
            assert(CurLambdaClass->isLambda());
            if (CurLambdaClass == PLambdaClass)
              SkipRetainExpansion = true;
            CurrentCtx = CurLambdaClass->getParent();
          }
        
        
          unsigned PartialDepth, PartialIndex;
          llvm::tie(PartialDepth, PartialIndex) = getDepthAndIndex(PartialPack);

          if (PartialDepth == Depth && PartialIndex == Index)
            if (!SkipRetainExpansion)
              RetainExpansion = true;
        }
      }
    }
    
    if (!NumExpansions) {
      // The is the first pack we've seen for which we have an argument. 
      // Record it.
      NumExpansions = NewPackSize;
      FirstPack.first = Name;
      FirstPack.second = i->second;
      HaveFirstPack = true;
      continue;
    }
    
    if (NewPackSize != *NumExpansions) {
      // C++0x [temp.variadic]p5:
      //   All of the parameter packs expanded by a pack expansion shall have 
      //   the same number of arguments specified.
      if (HaveFirstPack)
        Diag(EllipsisLoc, diag::err_pack_expansion_length_conflict)
          << FirstPack.first << Name << *NumExpansions << NewPackSize
          << SourceRange(FirstPack.second) << SourceRange(i->second);
      else
        Diag(EllipsisLoc, diag::err_pack_expansion_length_conflict_multilevel)
          << Name << *NumExpansions << NewPackSize
          << SourceRange(i->second);
      return true;
    }
  }
  
  return false;
}

llvm::Optional<unsigned> Sema::getNumArgumentsInExpansion(QualType T,
                          const MultiLevelTemplateArgumentList &TemplateArgs) {
  QualType Pattern = cast<PackExpansionType>(T)->getPattern();
  SmallVector<UnexpandedParameterPack, 2> Unexpanded;
  CollectUnexpandedParameterPacksVisitor(Unexpanded).TraverseType(Pattern);

  llvm::Optional<unsigned> Result;
  for (unsigned I = 0, N = Unexpanded.size(); I != N; ++I) {
    // Compute the depth and index for this parameter pack.
    unsigned Depth;
    unsigned Index;
    
    if (const TemplateTypeParmType *TTP
          = Unexpanded[I].first.dyn_cast<const TemplateTypeParmType *>()) {
      Depth = TTP->getDepth();
      Index = TTP->getIndex();
    } else {      
      NamedDecl *ND = Unexpanded[I].first.get<NamedDecl *>();
      if (isa<ParmVarDecl>(ND)) {
        // Function parameter pack.
        typedef LocalInstantiationScope::DeclArgumentPack DeclArgumentPack;
        
        llvm::PointerUnion<Decl *, DeclArgumentPack *> *Instantiation
          = CurrentInstantiationScope->findInstantiationOf(
                                        Unexpanded[I].first.get<NamedDecl *>());
        if (Instantiation->is<Decl*>())
          // The pattern refers to an unexpanded pack. We're not ready to expand
          // this pack yet.
          return llvm::Optional<unsigned>();

        unsigned Size = Instantiation->get<DeclArgumentPack *>()->size();
        assert((!Result || *Result == Size) && "inconsistent pack sizes");
        Result = Size;
        continue;
      }
      
      llvm::tie(Depth, Index) = getDepthAndIndex(ND);        
    }
    if (Depth >= TemplateArgs.getNumLevels() ||
        !TemplateArgs.hasTemplateArgument(Depth, Index))
      // The pattern refers to an unknown template argument. We're not ready to
      // expand this pack yet.
      return llvm::Optional<unsigned>();
    
    // Determine the size of the argument pack.
    unsigned Size = TemplateArgs(Depth, Index).pack_size();
    assert((!Result || *Result == Size) && "inconsistent pack sizes");
    Result = Size;
  }
  
  return Result;
}

bool Sema::containsUnexpandedParameterPacks(Declarator &D) {
  const DeclSpec &DS = D.getDeclSpec();
  switch (DS.getTypeSpecType()) {
  case TST_typename:
  case TST_typeofType:
  case TST_underlyingType:
  case TST_atomic: {
    QualType T = DS.getRepAsType().get();
    if (!T.isNull() && T->containsUnexpandedParameterPack())
      return true;
    break;
  }
      
  case TST_typeofExpr:
  case TST_decltype:
    if (DS.getRepAsExpr() && 
        DS.getRepAsExpr()->containsUnexpandedParameterPack())
      return true;
    break;
      
  case TST_unspecified:
  case TST_void:
  case TST_char:
  case TST_wchar:
  case TST_char16:
  case TST_char32:
  case TST_int:
  case TST_int128:
  case TST_half:
  case TST_float:
  case TST_double:
  case TST_bool:
  case TST_decimal32:
  case TST_decimal64:
  case TST_decimal128:
  case TST_enum:
  case TST_union:
  case TST_struct:
  case TST_interface:
  case TST_class:
  case TST_auto:
  case TST_unknown_anytype:
  case TST_error:
    break;
  }
  
  for (unsigned I = 0, N = D.getNumTypeObjects(); I != N; ++I) {
    const DeclaratorChunk &Chunk = D.getTypeObject(I);
    switch (Chunk.Kind) {
    case DeclaratorChunk::Pointer:
    case DeclaratorChunk::Reference:
    case DeclaratorChunk::Paren:
      // These declarator chunks cannot contain any parameter packs.
      break;
        
    case DeclaratorChunk::Array:
    case DeclaratorChunk::Function:
    case DeclaratorChunk::BlockPointer:
      // Syntactically, these kinds of declarator chunks all come after the
      // declarator-id (conceptually), so the parser should not invoke this
      // routine at this time.
      llvm_unreachable("Could not have seen this kind of declarator chunk");
        
    case DeclaratorChunk::MemberPointer:
      if (Chunk.Mem.Scope().getScopeRep() &&
          Chunk.Mem.Scope().getScopeRep()->containsUnexpandedParameterPack())
        return true;
      break;
    }
  }
  
  return false;
}

namespace {

// Callback to only accept typo corrections that refer to parameter packs.
class ParameterPackValidatorCCC : public CorrectionCandidateCallback {
 public:
  virtual bool ValidateCandidate(const TypoCorrection &candidate) {
    NamedDecl *ND = candidate.getCorrectionDecl();
    return ND && ND->isParameterPack();
  }
};

}

/// \brief Called when an expression computing the size of a parameter pack
/// is parsed.
///
/// \code
/// template<typename ...Types> struct count {
///   static const unsigned value = sizeof...(Types);
/// };
/// \endcode
///
//
/// \param OpLoc The location of the "sizeof" keyword.
/// \param Name The name of the parameter pack whose size will be determined.
/// \param NameLoc The source location of the name of the parameter pack.
/// \param RParenLoc The location of the closing parentheses.
ExprResult Sema::ActOnSizeofParameterPackExpr(Scope *S,
                                              SourceLocation OpLoc,
                                              IdentifierInfo &Name,
                                              SourceLocation NameLoc,
                                              SourceLocation RParenLoc) {
  // C++0x [expr.sizeof]p5:
  //   The identifier in a sizeof... expression shall name a parameter pack.
  LookupResult R(*this, &Name, NameLoc, LookupOrdinaryName);
  LookupName(R, S);
  
  NamedDecl *ParameterPack = 0;
  ParameterPackValidatorCCC Validator;
  switch (R.getResultKind()) {
  case LookupResult::Found:
    ParameterPack = R.getFoundDecl();
    break;
    
  case LookupResult::NotFound:
  case LookupResult::NotFoundInCurrentInstantiation:
    if (TypoCorrection Corrected = CorrectTypo(R.getLookupNameInfo(),
                                               R.getLookupKind(), S, 0,
                                               Validator)) {
      std::string CorrectedQuotedStr(Corrected.getQuoted(getLangOpts()));
      ParameterPack = Corrected.getCorrectionDecl();
      Diag(NameLoc, diag::err_sizeof_pack_no_pack_name_suggest)
        << &Name << CorrectedQuotedStr
        << FixItHint::CreateReplacement(
            NameLoc, Corrected.getAsString(getLangOpts()));
      Diag(ParameterPack->getLocation(), diag::note_parameter_pack_here)
        << CorrectedQuotedStr;
    }
      
  case LookupResult::FoundOverloaded:
  case LookupResult::FoundUnresolvedValue:
    break;
    
  case LookupResult::Ambiguous:
    DiagnoseAmbiguousLookup(R);
    return ExprError();
  }
  
  if (!ParameterPack || !ParameterPack->isParameterPack()) {
    Diag(NameLoc, diag::err_sizeof_pack_no_pack_name)
      << &Name;
    return ExprError();
  }

  MarkAnyDeclReferenced(OpLoc, ParameterPack);

  return new (Context) SizeOfPackExpr(Context.getSizeType(), OpLoc, 
                                      ParameterPack, NameLoc, RParenLoc);
}
