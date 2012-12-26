//===--- SemaLambda.cpp - Semantic Analysis for C++11 Lambdas -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file implements semantic analysis for C++ lambda expressions.
//
//===----------------------------------------------------------------------===//
#include "clang/Sema/DeclSpec.h"
#include "clang/Sema/Initialization.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/ScopeInfo.h"
#include "clang/Sema/SemaInternal.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/TypeLoc.h"

#include "clang/Sema/Template.h"
#include "TreeTransform.h"
#include "clang/AST/RecursiveASTVisitor.h" 

using namespace clang;
using namespace sema;

CXXRecordDecl *Sema::createLambdaClosureType(SourceRange IntroducerRange,
                                             TypeSourceInfo *Info,
                                             bool KnownDependent) {
  DeclContext *DC = CurContext;
  while (!(DC->isFunctionOrMethod() || DC->isRecord() || DC->isFileContext()))
    DC = DC->getParent();
  
  // Start constructing the lambda class.
  CXXRecordDecl *Class = CXXRecordDecl::CreateLambda(Context, DC, Info,
                                                     IntroducerRange.getBegin(),
                                                     KnownDependent);
  DC->addDecl(Class);
  
  return Class;
}

/// \brief Determine whether the given context is or is enclosed in an inline
/// function.
static bool isInInlineFunction(const DeclContext *DC) {
  while (!DC->isFileContext()) {
    if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(DC))
      if (FD->isInlined())
        return true;
    
    DC = DC->getLexicalParent();
  }
  
  return false;
}


// Check if there is an auto parameter - and if so, we have a generic lambda
static bool containsAutoParameter( 
      llvm::ArrayRef<ParmVarDecl *> Params)
{
  bool HasGenericAutoParameter = false;
  for (size_t i = 0; i < Params.size(); ++i)
  {
    // Get the type*, this contains the full info i.e. auto*, int& etc.
    const Type* ty = Params[i]->getOriginalType().getTypePtr(); 
    // This only returns non-null if 'auto' is within the type pattern
    AutoType* at = ty->getContainedAutoType(); 
    if (at)
    {
      HasGenericAutoParameter = true;
      break;
    }
  }
  return HasGenericAutoParameter;
}

namespace {

  /// Substitute the 'auto' type specifier within a type for a given replacement
  /// type.
  class SubstituteAutoTransform :
    public TreeTransform<SubstituteAutoTransform> {
    QualType Replacement;
  public:
    SubstituteAutoTransform(Sema &SemaRef, QualType Replacement) :
      TreeTransform<SubstituteAutoTransform>(SemaRef), Replacement(Replacement) {
    }
    QualType TransformAutoType(TypeLocBuilder &TLB, AutoTypeLoc TL) {
      // If we're building the type pattern to deduce against, don't wrap the
      // substituted type in an AutoType. Certain template deduction rules
      // apply only when a template type parameter appears directly (and not if
      // the parameter is found through desugaring). For instance:
      //   auto &&lref = lvalue;
      // must transform into "rvalue reference to T" not "rvalue reference to
      // auto type deduced as T" in order for [temp.deduct.call]p3 to apply.
      if (isa<TemplateTypeParmType>(Replacement)) {
        QualType Result = Replacement;
        TemplateTypeParmTypeLoc NewTL = TLB.push<TemplateTypeParmTypeLoc>(Result);
        NewTL.setNameLoc(TL.getNameLoc());
        return Result;
      } else {
        QualType Result = RebuildAutoType(Replacement);
        AutoTypeLoc NewTL = TLB.push<AutoTypeLoc>(Result);
        NewTL.setNameLoc(TL.getNameLoc());
        return Result;
      }
    }

    ExprResult TransformLambdaExpr(LambdaExpr *E) {
      // Lambdas never need to be transformed.
      return E;
    }
  };
} // End Namespace anon


// Check if this param references a prior parameter.
// (i.e.decltype (a) b) and rewire it to refer to the new invented 
// parameter - otherwise it will not be linked to the corresponding
// template parameter during instantiation.
    
// Since we can not be sure of how deeply nested the reference 
// might be, or whether we are referring to multiple previous params
// we use a Visitor to comprehensively adjust all the pointers.
   
// If we find any references to the prior parameters we use the new 
// renditions that we invented above (i.e. those not containing 'auto') 
// instead
// i.e. [](auto a, auto* a2, decltype(a) b, X<sizeof(a)> x, 
//          Y<decltype(a)> y,  int c, Z<decltype(x)> z, 
//          auto (*b)(decltype(a), decltype(a2)) ) 
//
// FVQUESTION: Is it safe to just reset the Decl within the DeclRefExpr
//   of the old parameter to the new one?
//   2) Is using the visitor the best way to do this?
       
struct FixReferencesToPreviousParametersVisitor
  : RecursiveASTVisitor<FixReferencesToPreviousParametersVisitor> {
          
    // The Parameter Index we should not go beyond 
    // can be the full size when processing the return type
    const size_t CurMaxIndex;
    const llvm::ArrayRef<ParmVarDecl *> &OriginalParams;
    const SmallVector<ParmVarDecl*, 4>  &NewParams;
  public:
    FixReferencesToPreviousParametersVisitor(size_t CurMaxIndex,
        const llvm::ArrayRef<ParmVarDecl *> &OriginalParams,
        const SmallVector<ParmVarDecl*, 4>  &NewParams) :
        CurMaxIndex(CurMaxIndex), OriginalParams(OriginalParams),
        NewParams(NewParams) { }
    // Get the Decl referenced by the expression within the
    // parameter, and check to see if it needs to be 
    // adjusted           
    bool VisitDeclRefExpr(DeclRefExpr *DRE) {
      ValueDecl* VD = DRE->getDecl();
      for (size_t i = 0; i < CurMaxIndex; ++i )
      {
        if (VD == cast<ValueDecl>(OriginalParams[i]))
          DRE->setDecl(NewParams[i]);
      } 
      return true;
    }
};
   

// Takes the parameters of a generic lambda, checks them 
// for the presence of Auto, and generates corresponding
// parameters with an appropraite tempalte parameter list
//FVTODO This function will need to be cleaned up and refactored
template<class ContainerT1, class ContainerT2, class ContainerT3>
static void createTemplateVersionsOfAutoParameters(
                              CXXRecordDecl *Class,
                              const ContainerT1 &Params,       //OriginalParamsIn,
                              ContainerT2 &TemplateParams,             //TemplateTypeParametersOut,
                              TemplateParameterList *OrigTemplateParamList,
                              ContainerT3 &FuncParamsWithAutoReplaced,
                              unsigned int TemplateParamDepth,
                              Sema &S, QualType *ResultTypePtr)            //NewParamsWithAutoReplacedOut)
{
  ASTContext &Context = S.Context; 
  QualType &ResultType = *ResultTypePtr;
  // if this generic lambda has its own template
  // parameter list, add it first to our
  // template params
  // i.e. []<class T, int N>(T (&a)[N], auto b ) ...
  if (OrigTemplateParamList) {
    for (size_t i = 0; i < OrigTemplateParamList->size(); ++i)
      TemplateParams.push_back(OrigTemplateParamList->getParam(i));
  }
  // The depth of the template parameter list we will create
  //unsigned int TemplateParamDepth = OrigTemplateParamList ? 
  //                OrigTemplateParamList->getDepth() : 0;
  for (size_t CurrentParameterIndex = 0; 
          CurrentParameterIndex < Params.size(); 
          ++CurrentParameterIndex)
  {
    ParmVarDecl* AutoParam = Params[CurrentParameterIndex];
    QualType AutoParamQType = AutoParam->getOriginalType();
    ParmVarDecl* OrigOrNewParam = AutoParam;
    // get the the full param info i.e. auto* etc.
    const Type* AutoParamType = AutoParamQType.getTypePtr(); 
   
    if (AutoParamType->getContainedAutoType())
    {  
      // FVQUESTION? We might not need to invent a parameter identifier 
      // for this invented parameter
      // Is there a way to get clang to generate a unique ID?
      // Currently we just ask the IdentifierTable for the ID
      // that we generate - this becomes the name of our 
      // "template type"
      //  Is it safe to prefix with _$n ?
      // Also should we add the template parameter depth to the name?
      std::string InventedTemplateParamName = "_$";
      llvm::raw_string_ostream ss(InventedTemplateParamName);
      ss << CurrentParameterIndex;  // identify unnamed [](auto, auto) by idx
      ss << AutoParam->getNameAsString();
      ss.flush();

      IdentifierInfo& TemplateParamII = Context.Idents.get(
                                  InventedTemplateParamName.c_str());
      
      // Invent a template type parameter - corresponding to the Auto 
      // containing parameters
      TemplateTypeParmDecl *TemplateParam =
        TemplateTypeParmDecl::Create(Context, 
                  Class, SourceLocation(), 
                  AutoParam->getLocation(), 
                  TemplateParamDepth, 
                  TemplateParams.size(),  // our template param index 
                  &TemplateParamII, false, false);
      
      TemplateParams.push_back(TemplateParam);

      // Now replace the 'auto' in the function parameter
      // with this invented type name
      // getTypeForDecl returns the InventedTemplateParamName as a type
      QualType TemplParamType = QualType(TemplateParam->getTypeForDecl(), 0);
      
      // FVQUESTION? Should I be passing in the location of the original parameter??
      //   Is this the best way to create the TypeSourceInfo that will 
      //     be transformed from auto a -> _$a a
      TypeSourceInfo* AutoParamTSI = Context.getTrivialTypeSourceInfo(
                                          AutoParam->getType(), 
                                          AutoParam->getLocation());  
      TypeSourceInfo *FuncParamWithAutoReplacedTSI =
          SubstituteAutoTransform(S, TemplParamType).TransformType(AutoParamTSI);


      QualType FuncParamWithAutoReplaced = 
                              FuncParamWithAutoReplacedTSI->getType();
      
      // create a new parameter with 'auto' replaced by an invented 
      // template parameter type such as _$a
      ParmVarDecl* FuncParamWithAutoReplacedDecl = ParmVarDecl::Create(
                      Context, AutoParam->getDeclContext(), 
                      AutoParam->getLocStart(), AutoParam->getLocation(),
                      AutoParam->getIdentifier(), FuncParamWithAutoReplaced, 
                      FuncParamWithAutoReplacedTSI, AutoParam->getStorageClass(),
                      AutoParam->getStorageClassAsWritten(), AutoParam->getDefaultArg());
 
      // Set the depth and index of this parameter 
      int ParamDepth = AutoParam->getFunctionScopeDepth();
      int ParamIndex = AutoParam->getFunctionScopeIndex();
      FuncParamWithAutoReplacedDecl->setScopeInfo(
                                ParamDepth, ParamIndex);  
      
      FuncParamsWithAutoReplaced.push_back( FuncParamWithAutoReplacedDecl );
      OrigOrNewParam = FuncParamWithAutoReplacedDecl;
    } 
    else
    {
      // Since there was no auto within this parameter, 
      // don't transform it.

      FuncParamsWithAutoReplaced.push_back(OrigOrNewParam);
    }
       
    FixReferencesToPreviousParametersVisitor visitor(CurrentParameterIndex + 1, 
                                Params, FuncParamsWithAutoReplaced);
    visitor.TraverseDecl(OrigOrNewParam);
    // now that we have gone through all the parameters, fix the result
    // type if it refers to any previous parameter, ie. 
    // [](auto a) -> decltype(a) or -> X<decltype(a)>
    if (CurrentParameterIndex == Params.size() - 1)
      visitor.TraverseType(ResultType);  
  }
 
}

// FVTODO: This function needs to be refactored and cleaned up
// create a member template within the Closure class
// with each use of auto generating a corresponding template type
// parameter
static CXXMethodDecl* createGenericLambdaMethod(CXXRecordDecl *Class,
                            SourceRange IntroducerRange,
                            TypeSourceInfo *OrigTSI,
                            SourceLocation EndLoc,
                            llvm::ArrayRef<ParmVarDecl *> Params,
                            TemplateParameterList *OrigTemplateParamList,
                            unsigned int TemplateParameterDepth,
                            Sema &S) {
  
  ASTContext &Context = S.Context;

  const FunctionProtoType *AutoMethodFPT = 
              OrigTSI->getType()->castAs<FunctionProtoType>();

  // Copy the function prototype info (i.e. const, trailing return)
  // from the original declaration
  FunctionProtoType::ExtProtoInfo OrigFunctionInfo = 
                              AutoMethodFPT->getExtProtoInfo();

  // If there is no trailing return type, make the return
  // type auto - so we can use the C++1y auto deduction for all functions
  // feature to deduce the return type
  QualType ResultType;
  if (OrigFunctionInfo.HasTrailingReturn)
    ResultType = AutoMethodFPT->getResultType();//.getCanonicalType();
  else
    ResultType = Context.getAutoType(ResultType);

  SmallVector<NamedDecl*, 4> TemplateParams;
  SmallVector<ParmVarDecl*, 4> FuncParamsWithAutoReplaced;
  
  Scope *scope = S.getCurScope();
  unsigned int ActualDepth = S.getTemplateParameterDepth(
                                          cast<DeclContext>(Class));
  assert((!OrigTemplateParamList ||
      OrigTemplateParamList->getDepth() == ActualDepth)
      && "The depth of the explicit template parameter list, "
      " and the calculated depth from the lambda class should be the same");
  // Construct a template parameter list, corresponding to each
  // auto parameter
  // (auto a, auto* b) ==>
  //   template<class $a, class $b> ($a a, $b* b)
  createTemplateVersionsOfAutoParameters(Class, 
                                      Params, TemplateParams, 
                                      OrigTemplateParamList,
                                      FuncParamsWithAutoReplaced, 
                                      ActualDepth, S, &ResultType);

  // Create the corresponding template parameter list
  //  with the invented parameter types for each use of auto
  SourceLocation LAngleLoc, RAngleLoc = EndLoc;
  if (OrigTemplateParamList) {
    LAngleLoc = OrigTemplateParamList->getLAngleLoc();
  }
  TemplateParameterList* InventedTemplateParamList = 
                  TemplateParameterList::Create(Context, 
                  /* Template kw loc */ SourceLocation(), 
                                        LAngleLoc,
                                       (NamedDecl**)TemplateParams.data(), 
                                       TemplateParams.size(), RAngleLoc);

  // Now create the appropriate TypeSourceInfo for the function prototype
  // i.e. (auto a, auto* b) with (_$a a, _$b* b)
  
  // Create a vector of the invented QualType of each parameter 
  SmallVector<QualType,16> NewParamsType;
  for (size_t i = 0; i < FuncParamsWithAutoReplaced.size(); ++i)
  {
    NewParamsType.push_back(FuncParamsWithAutoReplaced[i]->getType());
  }

  // Create the Type of the new invented function
  QualType FunctionTypeWithAutoReplaced = S.Context.getFunctionType(
    ResultType, 
    NewParamsType.data(),
    FuncParamsWithAutoReplaced.size(), OrigFunctionInfo); 
  
  
  // Construct a new FunctionProtoType using the TypeLoc builder
  // which basically wires up the locations with the new function type

  // We will be using the original type loc info to substitute
  // into the new type loc info
  TypeLoc OrigTL = OrigTSI->getTypeLoc();
  FunctionProtoTypeLoc* OrigFPTLoc = dyn_cast<FunctionProtoTypeLoc>(&OrigTL);

  // Create the type builder and reserve all the space we needed
  // in the original function prototype loc
  TypeLocBuilder TLB;
  TLB.reserve(OrigTL.getFullDataSize());

  // When building the new FunctionProtoTypeLoc, we first have to add the
  // return type ... - we use the End Loc, because it becomes the EndLoc
  // of our eventual newFunctionProtoTypeLoc
  TypeSourceInfo *ResultTSI = Context.getTrivialTypeSourceInfo(ResultType, 
                                                        OrigTL.getEndLoc());
  TLB.pushFullCopy(ResultTSI->getTypeLoc());
  
  // Now push the function type - the return types will be checked
  FunctionProtoTypeLoc NewTL = TLB.push<FunctionProtoTypeLoc>(
                                            FunctionTypeWithAutoReplaced);
  NewTL.setLocalRangeBegin(OrigFPTLoc->getLocalRangeBegin());
  NewTL.setLParenLoc(OrigFPTLoc->getLParenLoc());
  NewTL.setRParenLoc(OrigFPTLoc->getRParenLoc());
  NewTL.setLocalRangeEnd(OrigFPTLoc->getLocalRangeEnd());
  // Add the parameter types
  for (unsigned i = 0, e = NewTL.getNumArgs(); i != e; ++i)
    NewTL.setArg(i, FuncParamsWithAutoReplaced[i]);
  TypeSourceInfo* NewTSI = 	TLB.getTypeSourceInfo (Context, 
                                        FunctionTypeWithAutoReplaced);
  // FVTODO: Introduce some assertions here to ensure that NewTSI
  //  has maintained the invariants from OrigTSI

  // FVTODO: Use TreeTransform and TransformType and possibly TemplateInstantiator
  // to transform decltype(a) in the return type by having 'a' refer to
  // the new ParmVarDecl

  // Get the DeclarationName associated with the function call operator
  DeclarationName MethodName
    = Context.DeclarationNames.getCXXOperatorName(OO_Call);
  DeclarationNameLoc MethodNameLoc;
  MethodNameLoc.CXXOperatorName.BeginOpNameLoc
    = IntroducerRange.getBegin().getRawEncoding();
  MethodNameLoc.CXXOperatorName.EndOpNameLoc
    = IntroducerRange.getEnd().getRawEncoding();
  CXXMethodDecl *Method
    = CXXMethodDecl::Create(Context, Class, EndLoc,
                            DeclarationNameInfo(MethodName, 
                                                IntroducerRange.getBegin(),
                                                MethodNameLoc),
                            NewTSI->getType(), NewTSI,
                            /*isStatic=*/false,
                            SC_None,
                            /*isInline=*/true,
                            /*isConstExpr=*/false,
                            EndLoc);
  
  Method->setAccess(AS_public);
  Method->setLexicalDeclContext(S.CurContext);
  
  FunctionTemplateDecl* TemplateMethod = 
                  FunctionTemplateDecl::Create(Context, Class,
                         Method->getLocation(), MethodName, 
                         InventedTemplateParamList,
                                                  Method);
  TemplateMethod->setLexicalDeclContext(S.CurContext);
  TemplateMethod->setAccess(AS_public);
  
  Method->setDescribedFunctionTemplate(TemplateMethod);

  // Add parameters.
  if (!FuncParamsWithAutoReplaced.empty()) {
    Method->setParams(FuncParamsWithAutoReplaced);
    S.CheckParmsForFunctionDef(
              const_cast<ParmVarDecl **>(
                            FuncParamsWithAutoReplaced.begin()),
              const_cast<ParmVarDecl **>(
                            FuncParamsWithAutoReplaced.end()),
                             /*CheckParameterNames=*/false);
    
    for (CXXMethodDecl::param_iterator P = Method->param_begin(), 
                                    PEnd = Method->param_end();
         P != PEnd; ++P)
      (*P)->setOwningFunction(Method);
  }
  Class->setGenericLambda(true);
  return Method;
}


static CXXMethodDecl* createNonGenericLambdaMethod(CXXRecordDecl *Class,
                                    SourceRange IntroducerRange,
                                    TypeSourceInfo *MethodType,
                                    SourceLocation EndLoc,
                                    llvm::ArrayRef<ParmVarDecl *> Params,
                                    Sema &S) {

  ASTContext& Context = S.Context;
  // C++11 [expr.prim.lambda]p5:
  //   The closure type for a lambda-expression has a public inline function 
  //   call operator (13.5.4) whose parameters and return type are described by
  //   the lambda-expression's parameter-declaration-clause and 
  //   trailing-return-type respectively.
  DeclarationName MethodName
    = Context.DeclarationNames.getCXXOperatorName(OO_Call);
  DeclarationNameLoc MethodNameLoc;
  MethodNameLoc.CXXOperatorName.BeginOpNameLoc
    = IntroducerRange.getBegin().getRawEncoding();
  MethodNameLoc.CXXOperatorName.EndOpNameLoc
    = IntroducerRange.getEnd().getRawEncoding();
  CXXMethodDecl *Method
    = CXXMethodDecl::Create(Context, Class, EndLoc,
    DeclarationNameInfo(MethodName, 
    IntroducerRange.getBegin(),
    MethodNameLoc),
    MethodType->getType(), MethodType,
    /*isStatic=*/false,
    SC_None,
    /*isInline=*/true,
    /*isConstExpr=*/false,
    EndLoc);
  Method->setAccess(AS_public);

  // Temporarily set the lexical declaration context to the current
  // context, so that the Scope stack matches the lexical nesting.
  Method->setLexicalDeclContext(S.CurContext);  

  // Add parameters.
  if (!Params.empty()) {
    Method->setParams(Params);
      S.CheckParmsForFunctionDef(const_cast<ParmVarDecl **>(Params.begin()),
      const_cast<ParmVarDecl **>(Params.end()),
      /*CheckParameterNames=*/false);

    for (CXXMethodDecl::param_iterator P = Method->param_begin(), 
      PEnd = Method->param_end();
      P != PEnd; ++P)
      (*P)->setOwningFunction(Method);
  }
  return Method;
}

CXXMethodDecl *Sema::startLambdaDefinition(CXXRecordDecl *Class,
  SourceRange IntroducerRange,
  TypeSourceInfo *MethodType,
  SourceLocation EndLoc,
  llvm::ArrayRef<ParmVarDecl *> Params,
  TemplateParameterList *TemplateParams,
  unsigned int TemplateParameterDepth) {


    bool IsGenericLambda = getLangOpts().GenericLambda &&
      ( TemplateParams || containsAutoParameter(Params) );
    
    CXXMethodDecl *Method = 0; 
    if (IsGenericLambda)
      Method = createGenericLambdaMethod(Class, IntroducerRange,
                                    MethodType, EndLoc, Params, 
                                    TemplateParams, TemplateParameterDepth, *this); 
    else
      Method = createNonGenericLambdaMethod(Class, IntroducerRange,
                                    MethodType, EndLoc, Params, *this);

    
    // Allocate a mangling number for this lambda expression, if the ABI
    // requires one.
    Decl *ContextDecl = ExprEvalContexts.back().LambdaContextDecl;

    enum ContextKind {
      Normal,
      DefaultArgument,
      DataMember,
      StaticDataMember
    } Kind = Normal;

    // Default arguments of member function parameters that appear in a class
    // definition, as well as the initializers of data members, receive special
    // treatment. Identify them.
    if (ContextDecl) {
      if (ParmVarDecl *Param = dyn_cast<ParmVarDecl>(ContextDecl)) {
        if (const DeclContext *LexicalDC
          = Param->getDeclContext()->getLexicalParent())
          if (LexicalDC->isRecord())
            Kind = DefaultArgument;
      } else if (VarDecl *Var = dyn_cast<VarDecl>(ContextDecl)) {
        if (Var->getDeclContext()->isRecord())
          Kind = StaticDataMember;
      } else if (isa<FieldDecl>(ContextDecl)) {
        Kind = DataMember;
      }
    }

    // Itanium ABI [5.1.7]:
    //   In the following contexts [...] the one-definition rule requires closure
    //   types in different translation units to "correspond":
    bool IsInNonspecializedTemplate =
      !ActiveTemplateInstantiations.empty() || CurContext->isDependentContext();
    unsigned ManglingNumber;
    switch (Kind) {
    case Normal:
      //  -- the bodies of non-exported nonspecialized template functions
      //  -- the bodies of inline functions
      if ((IsInNonspecializedTemplate &&
        !(ContextDecl && isa<ParmVarDecl>(ContextDecl))) ||
        isInInlineFunction(CurContext))
        ManglingNumber = Context.getLambdaManglingNumber(Method);
      else
        ManglingNumber = 0;

      // There is no special context for this lambda.
      ContextDecl = 0;
      break;

    case StaticDataMember:
      //  -- the initializers of nonspecialized static members of template classes
      if (!IsInNonspecializedTemplate) {
        ManglingNumber = 0;
        ContextDecl = 0;
        break;
      }
      // Fall through to assign a mangling number.

    case DataMember:
      //  -- the in-class initializers of class members
    case DefaultArgument:
      //  -- default arguments appearing in class definitions
      ManglingNumber = ExprEvalContexts.back().getLambdaMangleContext()
        .getManglingNumber(Method);
      break;
    }

    Class->setLambdaMangling(ManglingNumber, ContextDecl);

    return Method;
}


LambdaScopeInfo *Sema::enterLambdaScope(CXXMethodDecl *CallOperator,
                                        SourceRange IntroducerRange,
                                        LambdaCaptureDefault CaptureDefault,
                                        bool ExplicitParams,
                                        bool ExplicitResultType,
                                        bool Mutable) {
  PushLambdaScope(CallOperator->getParent(), CallOperator);
  LambdaScopeInfo *LSI = getCurLambda();
  if (CaptureDefault == LCD_ByCopy)
    LSI->ImpCaptureStyle = LambdaScopeInfo::ImpCap_LambdaByval;
  else if (CaptureDefault == LCD_ByRef)
    LSI->ImpCaptureStyle = LambdaScopeInfo::ImpCap_LambdaByref;
  LSI->IntroducerRange = IntroducerRange;
  LSI->ExplicitParams = ExplicitParams;
  LSI->Mutable = Mutable;

  if (ExplicitResultType) {
    LSI->ReturnType = CallOperator->getResultType();
    
    if (!LSI->ReturnType->isDependentType() &&
        !LSI->ReturnType->isVoidType()) {
      if (RequireCompleteType(CallOperator->getLocStart(), LSI->ReturnType,
                              diag::err_lambda_incomplete_result)) {
        // Do nothing.
      } else if (LSI->ReturnType->isObjCObjectOrInterfaceType()) {
        Diag(CallOperator->getLocStart(), diag::err_lambda_objc_object_result)
          << LSI->ReturnType;
      }
    }
  } else {
    LSI->HasImplicitReturnType = true;  
    // For implicitly deduced generic lambdas, the return type
    // is 'auto', so make sure that LSI->ReturnType
    // reflects that - 
    // FVTODO: this may seem unnecessary, but it is currently necessary
    if (getLangOpts().GenericLambda && 
                   CallOperator->getResultType()->getContainedAutoType())
      LSI->ReturnType = CallOperator->getResultType();
  }

  return LSI;
}

void Sema::finishLambdaExplicitCaptures(LambdaScopeInfo *LSI) {
  LSI->finishedExplicitCaptures();
}

void Sema::addLambdaParameters(CXXMethodDecl *CallOperator, Scope *CurScope) {  
  // Introduce our parameters into the function scope
  for (unsigned p = 0, NumParams = CallOperator->getNumParams(); 
       p < NumParams; ++p) {
    ParmVarDecl *Param = CallOperator->getParamDecl(p);
    
    // If this has an identifier, add it to the scope stack.
    if (CurScope && Param->getIdentifier()) {
      CheckShadow(CurScope, Param);
      
      PushOnScopeChains(Param, CurScope);
    }
  }
}

static bool checkReturnValueType(const ASTContext &Ctx, const Expr *E,
                                 QualType &DeducedType,
                                 QualType &AlternateType) {
  // Handle ReturnStmts with no expressions.
  if (!E) {
    if (AlternateType.isNull())
      AlternateType = Ctx.VoidTy;

    return Ctx.hasSameType(DeducedType, Ctx.VoidTy);
  }

  QualType StrictType = E->getType();
  QualType LooseType = StrictType;

  // In C, enum constants have the type of their underlying integer type,
  // not the enum. When inferring block return types, we should allow
  // the enum type if an enum constant is used, unless the enum is
  // anonymous (in which case there can be no variables of its type).
  if (!Ctx.getLangOpts().CPlusPlus) {
    const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E->IgnoreParenImpCasts());
    if (DRE) {
      const Decl *D = DRE->getDecl();
      if (const EnumConstantDecl *ECD = dyn_cast<EnumConstantDecl>(D)) {
        const EnumDecl *Enum = cast<EnumDecl>(ECD->getDeclContext());
        if (Enum->getDeclName() || Enum->getTypedefNameForAnonDecl())
          LooseType = Ctx.getTypeDeclType(Enum);
      }
    }
  }

  // Special case for the first return statement we find.
  // The return type has already been tentatively set, but we might still
  // have an alternate type we should prefer.
  if (AlternateType.isNull())
    AlternateType = LooseType;

  if (Ctx.hasSameType(DeducedType, StrictType)) {
    // FIXME: The loose type is different when there are constants from two
    // different enums. We could consider warning here.
    if (AlternateType != Ctx.DependentTy)
      if (!Ctx.hasSameType(AlternateType, LooseType))
        AlternateType = Ctx.VoidTy;
    return true;
  }

  if (Ctx.hasSameType(DeducedType, LooseType)) {
    // Use DependentTy to signal that we're using an alternate type and may
    // need to add casts somewhere.
    AlternateType = Ctx.DependentTy;
    return true;
  }

  if (Ctx.hasSameType(AlternateType, StrictType) ||
      Ctx.hasSameType(AlternateType, LooseType)) {
    DeducedType = AlternateType;
    // Use DependentTy to signal that we're using an alternate type and may
    // need to add casts somewhere.
    AlternateType = Ctx.DependentTy;
    return true;
  }

  return false;
}

// deduceClosureReturnType iterates through all the returns
// and makes sure there is coherence amongst all return types
// Remember that as each return statement is acted upon
// CSI.ReturnType gets set to the most recent
// return statement's return type (to aid with diagnostics)
// But that is a tentative return type of the function/lambda
void Sema::deduceClosureReturnType(CapturingScopeInfo &CSI) {
  assert(CSI.HasImplicitReturnType);

  // First case: no return statements, implicit void return type.
  ASTContext &Ctx = getASTContext();
  if (CSI.Returns.empty()) {
    // It's possible there were simply no /valid/ return statements.
    // In this case, the first one we found may have at least given us a type.
    
    // Do we really need this check here - can't we just assign to VoidTy ?
    if (CSI.ReturnType.isNull() || CSI.ReturnType->getContainedAutoType())
      CSI.ReturnType = Ctx.VoidTy;
    return;
  }

  // Second case: at least one return statement (should be the most recent, 
  //   unless the most recent was not well-formed and skipped so we
  //   are just diagnosticating then ...) has dependent type.
  // Delay type checking until instantiation.
  assert(!CSI.ReturnType.isNull() && "We should have a tentative return type.");
  if (CSI.ReturnType->isDependentType())
    return;
 
  // Third case: only one return statement. Don't bother doing extra work!
  SmallVectorImpl<ReturnStmt*>::iterator I = CSI.Returns.begin(),
                                         E = CSI.Returns.end();
  if (I+1 == E)
    return;

  // General case: many return statements.
  // Check that they all have compatible return types.
  // For now, that means "identical", with an exception for enum constants.
  // (In C, enum constants have the type of their underlying integer type,
  // not the type of the enum. C++ uses the type of the enum.)
  QualType AlternateType;

  // We require the return types to strictly match here.
  for (; I != E; ++I) {
    const ReturnStmt *RS = *I;
    const Expr *RetE = RS->getRetValue();
    if (!checkReturnValueType(Ctx, RetE, CSI.ReturnType, AlternateType)) {
      // FIXME: This is a poor diagnostic for ReturnStmts without expressions.
      Diag(RS->getLocStart(),
           diag::err_typecheck_missing_return_type_incompatible)
        << (RetE ? RetE->getType() : Ctx.VoidTy) << CSI.ReturnType
        << isa<LambdaScopeInfo>(CSI);
      // Don't bother fixing up the return statements in the block if some of
      // them are unfixable anyway.
      AlternateType = Ctx.VoidTy;
      // Continue iterating so that we keep emitting diagnostics.
    }
  }

  // If our return statements turned out to be compatible, but we needed to
  // pick a different return type, go through and fix the ones that need it.
  if (AlternateType == Ctx.DependentTy) {
    for (SmallVectorImpl<ReturnStmt*>::iterator I = CSI.Returns.begin(),
                                                E = CSI.Returns.end();
         I != E; ++I) {
      ReturnStmt *RS = *I;
      Expr *RetE = RS->getRetValue();
      if (RetE->getType() == CSI.ReturnType)
        continue;

      // Right now we only support integral fixup casts.
      assert(CSI.ReturnType->isIntegralOrUnscopedEnumerationType());
      assert(RetE->getType()->isIntegralOrUnscopedEnumerationType());
      ExprResult Casted = ImpCastExprToType(RetE, CSI.ReturnType,
                                            CK_IntegralCast);
      assert(Casted.isUsable());
      RS->setRetValue(Casted.take());
    }
  }
}


void Sema::ActOnStartOfLambdaDefinition(LambdaIntroducer &Intro,
                                        Declarator &ParamInfo,
                                        Scope *CurScope,
                                        TemplateParameterList *TemplateParams,
                                        unsigned int TemplateParameterDepth) {
  // Determine if we're within a context where we know that the lambda will
  // be dependent, because there are template parameters in scope.
  bool KnownDependent = false;
  if (Scope *TmplScope = CurScope->getTemplateParamParent())
  {
    // we have our own TemplateParams, so check if an outer scope
    // has template params, only then are we in a dependent scope
    // FVTODO: This needs to be able to work for 
    // []<class T>(T t) { return []<class T2>(T2 t2) { } }
    //  and i think it does 
    if (TemplateParams)  
    {
      TmplScope = TmplScope->getParent();
      TmplScope = TmplScope ? TmplScope->getTemplateParamParent() : 0;
    }
    if (TmplScope && !TmplScope->decl_empty())
      KnownDependent = true;
  }
  // Determine the signature of the call operator.
  TypeSourceInfo *MethodTyInfo;
  bool ExplicitParams = true;
  bool ExplicitResultType = true;
  bool ContainsUnexpandedParameterPack = false;
  SourceLocation EndLoc;
  llvm::SmallVector<ParmVarDecl *, 8> Params;
  if (ParamInfo.getNumTypeObjects() == 0) {
    // C++11 [expr.prim.lambda]p4:
    //   If a lambda-expression does not include a lambda-declarator, it is as 
    //   if the lambda-declarator were ().
    FunctionProtoType::ExtProtoInfo EPI;
    EPI.HasTrailingReturn = true;
    EPI.TypeQuals |= DeclSpec::TQ_const;
    QualType MethodTy = Context.getFunctionType(Context.DependentTy,
                                                /*Args=*/0, /*NumArgs=*/0, EPI);
    MethodTyInfo = Context.getTrivialTypeSourceInfo(MethodTy);
    ExplicitParams = false;
    ExplicitResultType = false;
    EndLoc = Intro.Range.getEnd();
  } else {
    assert(ParamInfo.isFunctionDeclarator() &&
           "lambda-declarator is a function");
    DeclaratorChunk::FunctionTypeInfo &FTI = ParamInfo.getFunctionTypeInfo();
    
    // C++11 [expr.prim.lambda]p5:
    //   This function call operator is declared const (9.3.1) if and only if 
    //   the lambda-expression's parameter-declaration-clause is not followed 
    //   by mutable. It is neither virtual nor declared volatile. [...]
    if (!FTI.hasMutableQualifier())
      FTI.TypeQuals |= DeclSpec::TQ_const;
    
    MethodTyInfo = GetTypeForDeclarator(ParamInfo, CurScope);
    assert(MethodTyInfo && "no type from lambda-declarator");
    EndLoc = ParamInfo.getSourceRange().getEnd();
    
    ExplicitResultType
      = MethodTyInfo->getType()->getAs<FunctionType>()->getResultType() 
                                                        != Context.DependentTy;

    if (FTI.NumArgs == 1 && !FTI.isVariadic && FTI.ArgInfo[0].Ident == 0 &&
        cast<ParmVarDecl>(FTI.ArgInfo[0].Param)->getType()->isVoidType()) {
      // Empty arg list, don't push any params.
      checkVoidParamDecl(cast<ParmVarDecl>(FTI.ArgInfo[0].Param));
    } else {
      Params.reserve(FTI.NumArgs);
      for (unsigned i = 0, e = FTI.NumArgs; i != e; ++i)
        Params.push_back(cast<ParmVarDecl>(FTI.ArgInfo[i].Param));
    }

    // Check for unexpanded parameter packs in the method type.
    if (MethodTyInfo->getType()->containsUnexpandedParameterPack())
      ContainsUnexpandedParameterPack = true;
  }

  CXXRecordDecl *Class = createLambdaClosureType(Intro.Range, MethodTyInfo,
                                                 KnownDependent);

  CXXMethodDecl *Method = startLambdaDefinition(Class, Intro.Range,
                                                MethodTyInfo, EndLoc, Params, 
                                                TemplateParams, TemplateParameterDepth);
  Class->setLambdaCallOperator(Method);
  if (ExplicitParams)
    CheckCXXDefaultArguments(Method);
  
  // Attributes on the lambda apply to the method.  
  ProcessDeclAttributes(CurScope, Method, ParamInfo);
  
  // Introduce the function call operator as the current declaration context.
  PushDeclContext(CurScope, Method);
    
  // Introduce the lambda scope.
  LambdaScopeInfo *LSI
    = enterLambdaScope(Method, Intro.Range, Intro.Default, ExplicitParams,
                       ExplicitResultType,
                       !Method->isConst());
 
  // Handle explicit captures.
  SourceLocation PrevCaptureLoc
    = Intro.Default == LCD_None? Intro.Range.getBegin() : Intro.DefaultLoc;
  for (llvm::SmallVector<LambdaCapture, 4>::const_iterator
         C = Intro.Captures.begin(), 
         E = Intro.Captures.end(); 
       C != E; 
       PrevCaptureLoc = C->Loc, ++C) {
    if (C->Kind == LCK_This) {
      // C++11 [expr.prim.lambda]p8:
      //   An identifier or this shall not appear more than once in a 
      //   lambda-capture.
      if (LSI->isCXXThisCaptured()) {
        Diag(C->Loc, diag::err_capture_more_than_once) 
          << "'this'"
          << SourceRange(LSI->getCXXThisCapture().getLocation())
          << FixItHint::CreateRemoval(
               SourceRange(PP.getLocForEndOfToken(PrevCaptureLoc), C->Loc));
        continue;
      }

      // C++11 [expr.prim.lambda]p8:
      //   If a lambda-capture includes a capture-default that is =, the 
      //   lambda-capture shall not contain this [...].
      if (Intro.Default == LCD_ByCopy) {
        Diag(C->Loc, diag::err_this_capture_with_copy_default)
          << FixItHint::CreateRemoval(
               SourceRange(PP.getLocForEndOfToken(PrevCaptureLoc), C->Loc));
        continue;
      }

      // C++11 [expr.prim.lambda]p12:
      //   If this is captured by a local lambda expression, its nearest
      //   enclosing function shall be a non-static member function.
      QualType ThisCaptureType = getCurrentThisType();
      if (ThisCaptureType.isNull()) {
        Diag(C->Loc, diag::err_this_capture) << true;
        continue;
      }
      
      CheckCXXThisCapture(C->Loc, /*Explicit=*/true);
      continue;
    }

    assert(C->Id && "missing identifier for capture");

    // C++11 [expr.prim.lambda]p8:
    //   If a lambda-capture includes a capture-default that is &, the 
    //   identifiers in the lambda-capture shall not be preceded by &.
    //   If a lambda-capture includes a capture-default that is =, [...]
    //   each identifier it contains shall be preceded by &.
    if (C->Kind == LCK_ByRef && Intro.Default == LCD_ByRef) {
      Diag(C->Loc, diag::err_reference_capture_with_reference_default)
        << FixItHint::CreateRemoval(
             SourceRange(PP.getLocForEndOfToken(PrevCaptureLoc), C->Loc));
      continue;
    } else if (C->Kind == LCK_ByCopy && Intro.Default == LCD_ByCopy) {
      Diag(C->Loc, diag::err_copy_capture_with_copy_default)
        << FixItHint::CreateRemoval(
             SourceRange(PP.getLocForEndOfToken(PrevCaptureLoc), C->Loc));
      continue;
    }

    DeclarationNameInfo Name(C->Id, C->Loc);
    LookupResult R(*this, Name, LookupOrdinaryName);
    LookupName(R, CurScope);
    if (R.isAmbiguous())
      continue;
    if (R.empty()) {
      // FIXME: Disable corrections that would add qualification?
      CXXScopeSpec ScopeSpec;
      DeclFilterCCC<VarDecl> Validator;
      if (DiagnoseEmptyLookup(CurScope, ScopeSpec, R, Validator))
        continue;
    }

    // C++11 [expr.prim.lambda]p10:
    //   The identifiers in a capture-list are looked up using the usual rules
    //   for unqualified name lookup (3.4.1); each such lookup shall find a 
    //   variable with automatic storage duration declared in the reaching 
    //   scope of the local lambda expression.
    // 
    // Note that the 'reaching scope' check happens in tryCaptureVariable().
    VarDecl *Var = R.getAsSingle<VarDecl>();
    if (!Var) {
      Diag(C->Loc, diag::err_capture_does_not_name_variable) << C->Id;
      continue;
    }

    // Ignore invalid decls; they'll just confuse the code later.
    if (Var->isInvalidDecl())
      continue;

    if (!Var->hasLocalStorage()) {
      Diag(C->Loc, diag::err_capture_non_automatic_variable) << C->Id;
      Diag(Var->getLocation(), diag::note_previous_decl) << C->Id;
      continue;
    }

    // C++11 [expr.prim.lambda]p8:
    //   An identifier or this shall not appear more than once in a 
    //   lambda-capture.
    if (LSI->isCaptured(Var)) {
      Diag(C->Loc, diag::err_capture_more_than_once) 
        << C->Id
        << SourceRange(LSI->getCapture(Var).getLocation())
        << FixItHint::CreateRemoval(
             SourceRange(PP.getLocForEndOfToken(PrevCaptureLoc), C->Loc));
      continue;
    }

    // C++11 [expr.prim.lambda]p23:
    //   A capture followed by an ellipsis is a pack expansion (14.5.3).
    SourceLocation EllipsisLoc;
    if (C->EllipsisLoc.isValid()) {
      if (Var->isParameterPack()) {
        EllipsisLoc = C->EllipsisLoc;
      } else {
        Diag(C->EllipsisLoc, diag::err_pack_expansion_without_parameter_packs)
          << SourceRange(C->Loc);
        
        // Just ignore the ellipsis.
      }
    } else if (Var->isParameterPack()) {
      ContainsUnexpandedParameterPack = true;
    }
    
    TryCaptureKind Kind = C->Kind == LCK_ByRef ? TryCapture_ExplicitByRef :
                                                 TryCapture_ExplicitByVal;
    tryCaptureVariable(Var, C->Loc, Kind, EllipsisLoc);
  }
  finishLambdaExplicitCaptures(LSI);

  LSI->ContainsUnexpandedParameterPack = ContainsUnexpandedParameterPack;

  // Add lambda parameters into scope.
  addLambdaParameters(Method, CurScope);

  // Enter a new evaluation context to insulate the lambda from any
  // cleanups from the enclosing full-expression.
  PushExpressionEvaluationContext(PotentiallyEvaluated);  
}

void Sema::ActOnLambdaError(SourceLocation StartLoc, Scope *CurScope,
                            bool IsInstantiation) {
  // Leave the expression-evaluation context.
  DiscardCleanupsInEvaluationContext();
  PopExpressionEvaluationContext();

  // Leave the context of the lambda.
  if (!IsInstantiation)
    PopDeclContext();

  // Finalize the lambda.
  LambdaScopeInfo *LSI = getCurLambda();
  CXXRecordDecl *Class = LSI->Lambda;
  Class->setInvalidDecl();
  SmallVector<Decl*, 4> Fields;
  for (RecordDecl::field_iterator i = Class->field_begin(),
                                  e = Class->field_end(); i != e; ++i)
    Fields.push_back(*i);
  ActOnFields(0, Class->getLocation(), Class, Fields, 
              SourceLocation(), SourceLocation(), 0);
  CheckCompletedCXXClass(Class);

  PopFunctionScopeInfo();
}

// For non-capturing generic lambdas, 
// we need to add a template conversion
// operator to function-ptr that returns the address of 
// a template static function (__invoke?) that will 
// forward the call to the corresponding generic
// function call operator.
// FVTODO: This needs to be refactored and cleaned up
static void addGenericFunctionPointerConversion(Sema &S, 
                                         SourceRange IntroducerRange,
                                         CXXRecordDecl *Class,
                                         CXXMethodDecl *CallOperator) {

  ASTContext &Context = S.Context;
  FunctionTemplateDecl* TemplateCallOperator = CallOperator->
                                        getDescribedFunctionTemplate();
  
  const FunctionProtoType *Proto
    = CallOperator->getType()->getAs<FunctionProtoType>(); 
  QualType FunctionPtrTy;
  QualType FunctionTy;
  {
    // Get rid of the member-const qualification since
    // it is irrelevant for the RETURN type of 
    // a non-capturing lambda's conversion to function pointer
    FunctionProtoType::ExtProtoInfo ExtInfo = Proto->getExtProtoInfo();
    ExtInfo.TypeQuals = 0;
    FunctionTy = S.Context.getFunctionType(Proto->getResultType(),
                                           Proto->arg_type_begin(),
                                           Proto->getNumArgs(),
                                           ExtInfo);
    FunctionPtrTy = S.Context.getPointerType(FunctionTy);
  }
  
  // The type of the conversion operator is still const
  FunctionProtoType::ExtProtoInfo ExtInfo;
  ExtInfo.TypeQuals = Qualifiers::Const;
  QualType ConvTy = S.Context.getFunctionType(FunctionPtrTy, 0, 0, ExtInfo);
  
  SourceLocation Loc = IntroducerRange.getBegin();
  DeclarationName Name
    = S.Context.DeclarationNames.getCXXConversionFunctionName(
        S.Context.getCanonicalType(FunctionPtrTy));
  DeclarationNameLoc NameLoc;
  NameLoc.NamedType.TInfo = S.Context.getTrivialTypeSourceInfo(FunctionPtrTy,
                                                               Loc);
  CXXConversionDecl *Conversion 
    = CXXConversionDecl::Create(S.Context, Class, Loc, 
                                DeclarationNameInfo(Name, Loc, NameLoc),
                                ConvTy, 
                                S.Context.getTrivialTypeSourceInfo(ConvTy, 
                                                                   Loc),
                                /*isInline=*/false, /*isExplicit=*/false,
                                /*isConstexpr=*/false, 
                                CallOperator->getBody()->getLocEnd());
  Conversion->setAccess(AS_public);
  Conversion->setImplicit(true);
 
  // Create a template version of the conversion operator, using the template 
  // parameter list of the function call operator
  FunctionTemplateDecl* TemplateConversion = 
                  FunctionTemplateDecl::Create(Context, Class,
                                      Loc, Name, 
                                      TemplateCallOperator->getTemplateParameters(),
                                      Conversion);
  // FVTODO: should this be private?
  TemplateConversion->setAccess(AS_public);

  Conversion->setDescribedFunctionTemplate(TemplateConversion);
  
  //FVTODO: Should we add the TemplateConversion
  Class->addDecl(TemplateConversion);

  Class->setLambdaConversionOperator(Conversion);

  // Add a non-static member function ?"__invoke" that will be the result of
  // the conversion.
  Name = &S.Context.Idents.get(Class->getGenericLambdaStaticInvokerString());
  CXXMethodDecl *Invoke
    = CXXMethodDecl::Create(S.Context, Class, Loc, 
                            DeclarationNameInfo(Name, Loc), FunctionTy, 
                            CallOperator->getTypeSourceInfo(),
                            /*IsStatic=*/true, SC_Static, /*IsInline=*/true,
                            /*IsConstexpr=*/false, 
                            CallOperator->getBody()->getLocEnd());
  // When we create parameters for our static-invoker, be mindful of 
  // the fact that parameters that refer to previous parameters
  // don't get re-wired - and if we try to rewire them via
  // re-setting the DeclRefExpr using a visitor, it affects the 
  // DeclRefExpr of the lambda call operator, since that does
  // not get cloned, and is shared.
  // Since static-invoker does not actually get called, we use 
  // a crappy kludge to get this to pass through instantiation 
  // i.e we instantiate the lambda call operator's params into
  // the instantiation scope of the static-invoker (special
  // casing AddressResolver to merge parent scopes when dealing)
  // with the special static invoker - ugh!
  // 
  // FVTODO: What would be nice is if I can figure out how to run 
  // a TreeTransform and clone all the parameters, with them
  // being completely rewired up to point to their correct 
  // declarations.
  // Anyways, until then heed this warning:
  // This way should be shut! It was made by those who are dead!
  // And the dead should bloody keep it...

  SmallVector<ParmVarDecl *, 4> InvokeParams;
  for (unsigned I = 0, N = CallOperator->getNumParams(); I != N; ++I) {
    ParmVarDecl *From = CallOperator->getParamDecl(I);
    ParmVarDecl *New = ParmVarDecl::Create(S.Context, Invoke,
                                               From->getLocStart(),
                                               From->getLocation(),
                                               From->getIdentifier(),
                                               From->getType(),
                                               From->getTypeSourceInfo(),
                                               From->getStorageClass(),
                                               From->getStorageClassAsWritten(),
                                               /*DefaultArg=*/0);
    New->setScopeInfo(From->getFunctionScopeDepth(), 
                          From->getFunctionScopeIndex());
    InvokeParams.push_back(New);
  }
  Invoke->setParams(InvokeParams);
  Invoke->setAccess(AS_public);
  Invoke->setImplicit(true);
   // Fill in the __invoke function with a dummy implementation. IR generation
  // will fill in the actual details.
  Invoke->setBody(new (S.Context) CompoundStmt(Loc));
  
  FunctionTemplateDecl* TemplateInvoke = 
                  FunctionTemplateDecl::Create(Context, Class,
                                      Loc, Name, 
                                      TemplateCallOperator->getTemplateParameters(),
                                      Invoke);
  TemplateInvoke->setAccess(AS_public);
  Invoke->setDescribedFunctionTemplate(TemplateInvoke);
  Class->addDecl(TemplateInvoke);
  Class->setLambdaStaticInvoker(Invoke);
}

/// \brief Add a lambda's conversion to function pointer, as described in
/// C++11 [expr.prim.lambda]p6.
static void addFunctionPointerConversion(Sema &S, 
                                         SourceRange IntroducerRange,
                                         CXXRecordDecl *Class,
                                         CXXMethodDecl *CallOperator) {
  if (Class->isGenericLambda())
    return addGenericFunctionPointerConversion(S, IntroducerRange,
                  Class, CallOperator);

  // Add the conversion to function pointer.
  const FunctionProtoType *Proto
    = CallOperator->getType()->getAs<FunctionProtoType>(); 
  QualType FunctionPtrTy;
  QualType FunctionTy;
  {
    FunctionProtoType::ExtProtoInfo ExtInfo = Proto->getExtProtoInfo();
    ExtInfo.TypeQuals = 0;
    FunctionTy = S.Context.getFunctionType(Proto->getResultType(),
                                           Proto->arg_type_begin(),
                                           Proto->getNumArgs(),
                                           ExtInfo);
    FunctionPtrTy = S.Context.getPointerType(FunctionTy);
  }
  
  FunctionProtoType::ExtProtoInfo ExtInfo;
  ExtInfo.TypeQuals = Qualifiers::Const;
  QualType ConvTy = S.Context.getFunctionType(FunctionPtrTy, 0, 0, ExtInfo);
  
  SourceLocation Loc = IntroducerRange.getBegin();
  DeclarationName Name
    = S.Context.DeclarationNames.getCXXConversionFunctionName(
        S.Context.getCanonicalType(FunctionPtrTy));
  DeclarationNameLoc NameLoc;
  NameLoc.NamedType.TInfo = S.Context.getTrivialTypeSourceInfo(FunctionPtrTy,
                                                               Loc);
  CXXConversionDecl *Conversion 
    = CXXConversionDecl::Create(S.Context, Class, Loc, 
                                DeclarationNameInfo(Name, Loc, NameLoc),
                                ConvTy, 
                                S.Context.getTrivialTypeSourceInfo(ConvTy, 
                                                                   Loc),
                                /*isInline=*/false, /*isExplicit=*/false,
                                /*isConstexpr=*/false, 
                                CallOperator->getBody()->getLocEnd());
  Conversion->setAccess(AS_public);
  Conversion->setImplicit(true);
  Class->addDecl(Conversion);
  Class->setLambdaConversionOperator(Conversion);
  // Add a non-static member function "__invoke" that will be the result of
  // the conversion.
  Name = &S.Context.Idents.get("__invoke");
  CXXMethodDecl *Invoke
    = CXXMethodDecl::Create(S.Context, Class, Loc, 
                            DeclarationNameInfo(Name, Loc), FunctionTy, 
                            CallOperator->getTypeSourceInfo(),
                            /*IsStatic=*/true, SC_Static, /*IsInline=*/true,
                            /*IsConstexpr=*/false, 
                            CallOperator->getBody()->getLocEnd());
  SmallVector<ParmVarDecl *, 4> InvokeParams;
  for (unsigned I = 0, N = CallOperator->getNumParams(); I != N; ++I) {
    ParmVarDecl *From = CallOperator->getParamDecl(I);
    InvokeParams.push_back(ParmVarDecl::Create(S.Context, Invoke,
                                               From->getLocStart(),
                                               From->getLocation(),
                                               From->getIdentifier(),
                                               From->getType(),
                                               From->getTypeSourceInfo(),
                                               From->getStorageClass(),
                                               From->getStorageClassAsWritten(),
                                               /*DefaultArg=*/0));
  }
  Invoke->setParams(InvokeParams);
  Invoke->setAccess(AS_private);
  Invoke->setImplicit(true);
  Class->addDecl(Invoke);
  Class->setLambdaStaticInvoker(Invoke);
}

/// \brief Add a lambda's conversion to block pointer.
static void addBlockPointerConversion(Sema &S, 
                                      SourceRange IntroducerRange,
                                      CXXRecordDecl *Class,
                                      CXXMethodDecl *CallOperator) {
  const FunctionProtoType *Proto
    = CallOperator->getType()->getAs<FunctionProtoType>(); 
  QualType BlockPtrTy;
  {
    FunctionProtoType::ExtProtoInfo ExtInfo = Proto->getExtProtoInfo();
    ExtInfo.TypeQuals = 0;
    QualType FunctionTy
      = S.Context.getFunctionType(Proto->getResultType(),
                                  Proto->arg_type_begin(),
                                  Proto->getNumArgs(),
                                  ExtInfo);
    BlockPtrTy = S.Context.getBlockPointerType(FunctionTy);
  }
  
  FunctionProtoType::ExtProtoInfo ExtInfo;
  ExtInfo.TypeQuals = Qualifiers::Const;
  QualType ConvTy = S.Context.getFunctionType(BlockPtrTy, 0, 0, ExtInfo);
  
  SourceLocation Loc = IntroducerRange.getBegin();
  DeclarationName Name
    = S.Context.DeclarationNames.getCXXConversionFunctionName(
        S.Context.getCanonicalType(BlockPtrTy));
  DeclarationNameLoc NameLoc;
  NameLoc.NamedType.TInfo = S.Context.getTrivialTypeSourceInfo(BlockPtrTy, Loc);
  CXXConversionDecl *Conversion 
    = CXXConversionDecl::Create(S.Context, Class, Loc, 
                                DeclarationNameInfo(Name, Loc, NameLoc),
                                ConvTy, 
                                S.Context.getTrivialTypeSourceInfo(ConvTy, Loc),
                                /*isInline=*/false, /*isExplicit=*/false,
                                /*isConstexpr=*/false, 
                                CallOperator->getBody()->getLocEnd());
  Conversion->setAccess(AS_public);
  Conversion->setImplicit(true);
  Class->addDecl(Conversion);
}
         
ExprResult Sema::ActOnLambdaExpr(SourceLocation StartLoc, Stmt *Body, 
                                 Scope *CurScope, 
                                 bool IsInstantiation) {
  // Collect information from the lambda scope.
  llvm::SmallVector<LambdaExpr::Capture, 4> Captures;
  llvm::SmallVector<Expr *, 4> CaptureInits;
  LambdaCaptureDefault CaptureDefault;
  CXXRecordDecl *Class;
  CXXMethodDecl *CallOperator;
  SourceRange IntroducerRange;
  bool ExplicitParams;
  bool ExplicitResultType;
  bool LambdaExprNeedsCleanups;
  bool ContainsUnexpandedParameterPack;
  llvm::SmallVector<VarDecl *, 4> ArrayIndexVars;
  llvm::SmallVector<unsigned, 4> ArrayIndexStarts;
  TemplateParameterList *TemplateParameters = 0;

  {
    LambdaScopeInfo *LSI = getCurLambda();
    CallOperator = LSI->CallOperator;
    Class = LSI->Lambda;
    IntroducerRange = LSI->IntroducerRange;
    ExplicitParams = LSI->ExplicitParams;
    ExplicitResultType = !LSI->HasImplicitReturnType;
    LambdaExprNeedsCleanups = LSI->ExprNeedsCleanups;
    ContainsUnexpandedParameterPack = LSI->ContainsUnexpandedParameterPack;
    ArrayIndexVars.swap(LSI->ArrayIndexVars);
    ArrayIndexStarts.swap(LSI->ArrayIndexStarts);
    
    // Translate captures.
    for (unsigned I = 0, N = LSI->Captures.size(); I != N; ++I) {
      LambdaScopeInfo::Capture From = LSI->Captures[I];
      assert(!From.isBlockCapture() && "Cannot capture __block variables");
      bool IsImplicit = I >= LSI->NumExplicitCaptures;

      // Handle 'this' capture.
      if (From.isThisCapture()) {
        Captures.push_back(LambdaExpr::Capture(From.getLocation(),
                                               IsImplicit,
                                               LCK_This));
        CaptureInits.push_back(new (Context) CXXThisExpr(From.getLocation(),
                                                         getCurrentThisType(),
                                                         /*isImplicit=*/true));
        continue;
      }

      VarDecl *Var = From.getVariable();
      LambdaCaptureKind Kind = From.isCopyCapture()? LCK_ByCopy : LCK_ByRef;
      Captures.push_back(LambdaExpr::Capture(From.getLocation(), IsImplicit, 
                                             Kind, Var, From.getEllipsisLoc()));
      CaptureInits.push_back(From.getCopyExpr());
    }

    switch (LSI->ImpCaptureStyle) {
    case CapturingScopeInfo::ImpCap_None:
      CaptureDefault = LCD_None;
      break;

    case CapturingScopeInfo::ImpCap_LambdaByval:
      CaptureDefault = LCD_ByCopy;
      break;

    case CapturingScopeInfo::ImpCap_LambdaByref:
      CaptureDefault = LCD_ByRef;
      break;

    case CapturingScopeInfo::ImpCap_Block:
      llvm_unreachable("block capture in lambda");
      break;
    }

    // C++11 [expr.prim.lambda]p4:
    //   If a lambda-expression does not include a
    //   trailing-return-type, it is as if the trailing-return-type
    //   denotes the following type:
    // FIXME: Assumes current resolution to core issue 975.
    if (LSI->HasImplicitReturnType) {
      deduceClosureReturnType(*LSI);

      //   - if there are no return statements in the
      //     compound-statement, or all return statements return
      //     either an expression of type void or no expression or
      //     braced-init-list, the type void;
      if (LSI->ReturnType.isNull()) {
        LSI->ReturnType = Context.VoidTy;
      }
      
      // FVTODO
      // for now, until we can figure out how LSI's ReturnType
      // gets switched to <dependent-type> during parsing of
      // the compound-statement that is the body
      // we need to check to see if this is a generic lambda
      // with an undeduced return type, and if it is
      // then leave it alone (since LSI's ReturnType gets
      //   out of sync i.e. becomes <dependent-type> and
      //   does not remain as auto - without this fix
      //   codegen/mangling of name becomes an issue - fairly
      //   sure i don't understand all the details here)
      
      // so if the return is not dependent or this is not
      // a generic lambda - reset the function type
      // with the deduced return type, which could
      // be <dependent type> - we disable this for 
      // generic lambdas,  because it messes with the 
      // 'auto' that we insert as the return type for 
      // generic lambdas - this will need to be made
      // symmetric for non-generics too
      if (!LSI->ReturnType->isDependentType() || 
        !Class->isGenericLambda())
      {
        // Create a function type with the inferred return type.
        const FunctionProtoType *Proto
          = CallOperator->getType()->getAs<FunctionProtoType>();
        QualType FunctionTy
          = Context.getFunctionType(LSI->ReturnType,
                                    Proto->arg_type_begin(),
                                    Proto->getNumArgs(),
                                    Proto->getExtProtoInfo());
        CallOperator->setType(FunctionTy);
      }
    }

    // C++ [expr.prim.lambda]p7:
    //   The lambda-expression's compound-statement yields the
    //   function-body (8.4) of the function call operator [...].
    ActOnFinishFunctionBody(CallOperator, Body, IsInstantiation);
    CallOperator->setLexicalDeclContext(Class);
    
    //FVADDED Add generic call operator, if it exists
    FunctionTemplateDecl* const GenericCallOperator = 
                CallOperator->getDescribedFunctionTemplate();
    
    if (GenericCallOperator)
      TemplateParameters = GenericCallOperator->getTemplateParameters();
    Decl *TheCallOp = GenericCallOperator ? 
            GenericCallOperator : static_cast<Decl*>(CallOperator);
    TheCallOp->setLexicalDeclContext(Class);
    Class->addDecl(TheCallOp);
   
    PopExpressionEvaluationContext();

    // C++11 [expr.prim.lambda]p6:
    //   The closure type for a lambda-expression with no lambda-capture
    //   has a public non-virtual non-explicit const conversion function
    //   to pointer to function having the same parameter and return
    //   types as the closure type's function call operator.
    if (Captures.empty() && CaptureDefault == LCD_None)
      addFunctionPointerConversion(*this, IntroducerRange, Class,
                                   CallOperator);

    // Objective-C++:
    //   The closure type for a lambda-expression has a public non-virtual
    //   non-explicit const conversion function to a block pointer having the
    //   same parameter and return types as the closure type's function call
    //   operator.
    if (getLangOpts().Blocks && getLangOpts().ObjC1)
      addBlockPointerConversion(*this, IntroducerRange, Class, CallOperator);
    
    // Finalize the lambda class.
    SmallVector<Decl*, 4> Fields;
    for (RecordDecl::field_iterator i = Class->field_begin(),
                                    e = Class->field_end(); i != e; ++i)
      Fields.push_back(*i);
    
    ActOnFields(0, Class->getLocation(), Class, Fields, 
                SourceLocation(), SourceLocation(), 0);
    CheckCompletedCXXClass(Class);
  }

  if (LambdaExprNeedsCleanups)
    ExprNeedsCleanups = true;
  
  LambdaExpr *Lambda = LambdaExpr::Create(Context, Class, IntroducerRange, 
                                          CaptureDefault, Captures, 
                                          ExplicitParams, ExplicitResultType,
                                          CaptureInits, ArrayIndexVars, 
                                          ArrayIndexStarts, Body->getLocEnd(),
                                          ContainsUnexpandedParameterPack,
                                          TemplateParameters);

  // C++11 [expr.prim.lambda]p2:
  //   A lambda-expression shall not appear in an unevaluated operand
  //   (Clause 5).
  if (!CurContext->isDependentContext()) {
    switch (ExprEvalContexts.back().Context) {
    case Unevaluated:
      // We don't actually diagnose this case immediately, because we
      // could be within a context where we might find out later that
      // the expression is potentially evaluated (e.g., for typeid).
      ExprEvalContexts.back().Lambdas.push_back(Lambda);
      break;

    case ConstantEvaluated:
    case PotentiallyEvaluated:
    case PotentiallyEvaluatedIfUsed:
      break;
    }
  }
  
  return MaybeBindToTemporary(Lambda);
}

ExprResult Sema::BuildBlockForLambdaConversion(SourceLocation CurrentLocation,
                                               SourceLocation ConvLocation,
                                               CXXConversionDecl *Conv,
                                               Expr *Src) {
  // Make sure that the lambda call operator is marked used.
  CXXRecordDecl *Lambda = Conv->getParent();
  CXXMethodDecl *CallOperator 
    = cast<CXXMethodDecl>(
        *Lambda->lookup(
          Context.DeclarationNames.getCXXOperatorName(OO_Call)).first);
  CallOperator->setReferenced();
  CallOperator->setUsed();

  ExprResult Init = PerformCopyInitialization(
                      InitializedEntity::InitializeBlock(ConvLocation, 
                                                         Src->getType(), 
                                                         /*NRVO=*/false),
                      CurrentLocation, Src);
  if (!Init.isInvalid())
    Init = ActOnFinishFullExpr(Init.take());
  
  if (Init.isInvalid())
    return ExprError();
  
  // Create the new block to be returned.
  BlockDecl *Block = BlockDecl::Create(Context, CurContext, ConvLocation);

  // Set the type information.
  Block->setSignatureAsWritten(CallOperator->getTypeSourceInfo());
  Block->setIsVariadic(CallOperator->isVariadic());
  Block->setBlockMissingReturnType(false);

  // Add parameters.
  SmallVector<ParmVarDecl *, 4> BlockParams;
  for (unsigned I = 0, N = CallOperator->getNumParams(); I != N; ++I) {
    ParmVarDecl *From = CallOperator->getParamDecl(I);
    BlockParams.push_back(ParmVarDecl::Create(Context, Block,
                                              From->getLocStart(),
                                              From->getLocation(),
                                              From->getIdentifier(),
                                              From->getType(),
                                              From->getTypeSourceInfo(),
                                              From->getStorageClass(),
                                            From->getStorageClassAsWritten(),
                                              /*DefaultArg=*/0));
  }
  Block->setParams(BlockParams);

  Block->setIsConversionFromLambda(true);

  // Add capture. The capture uses a fake variable, which doesn't correspond
  // to any actual memory location. However, the initializer copy-initializes
  // the lambda object.
  TypeSourceInfo *CapVarTSI =
      Context.getTrivialTypeSourceInfo(Src->getType());
  VarDecl *CapVar = VarDecl::Create(Context, Block, ConvLocation,
                                    ConvLocation, 0,
                                    Src->getType(), CapVarTSI,
                                    SC_None, SC_None);
  BlockDecl::Capture Capture(/*Variable=*/CapVar, /*ByRef=*/false,
                             /*Nested=*/false, /*Copy=*/Init.take());
  Block->setCaptures(Context, &Capture, &Capture + 1, 
                     /*CapturesCXXThis=*/false);

  // Add a fake function body to the block. IR generation is responsible
  // for filling in the actual body, which cannot be expressed as an AST.
  Block->setBody(new (Context) CompoundStmt(ConvLocation));

  // Create the block literal expression.
  Expr *BuildBlock = new (Context) BlockExpr(Block, Conv->getConversionType());
  ExprCleanupObjects.push_back(Block);
  ExprNeedsCleanups = true;

  return BuildBlock;
}
