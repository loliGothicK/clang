//===------- TemplateInstantiator.h - C++ Templates ---------------------*- C++ -*-===/
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//===----------------------------------------------------------------------===/
//
//  This file provides a TemplateInstantiator class that is used 
//  to substitute types into template patterns.
//
//===----------------------------------------------------------------------===/
#ifndef LLVM_CLANG_SEMA_TEMPLATE_INSTANTIATOR_H
#define LLVM_CLANG_SEMA_TEMPLATE_INSTANTIATOR_H

#include "clang/Sema/SemaInternal.h"
#include "TreeTransform.h"
#include "clang/Sema/DeclSpec.h"
#include "clang/Sema/Initialization.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Template.h"
#include "clang/Sema/TemplateDeduction.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/Basic/LangOptions.h"

#include "TreeTransform.h"


namespace clang {
class TemplateInstantiator : public clang::TreeTransform<TemplateInstantiator> {


  const MultiLevelTemplateArgumentList &TemplateArgs;
  SourceLocation Loc;
  DeclarationName Entity;
public:
  typedef TreeTransform<TemplateInstantiator> inherited;
  unsigned int getCurrentTemplateArgsLevel() const {
    return TemplateArgs.getNumLevels();
  }
  const MultiLevelTemplateArgumentList& 
    getDeducedTemplateArguments() const {
      return TemplateArgs;
  }

  TemplateInstantiator(Sema &SemaRef,
    const MultiLevelTemplateArgumentList &TemplateArgs,
    SourceLocation Loc,
    DeclarationName Entity)
    : inherited(SemaRef), TemplateArgs(TemplateArgs), Loc(Loc),
    Entity(Entity) { }

  /// \brief Determine whether the given type \p T has already been
  /// transformed.
  ///
  /// For the purposes of template instantiation, a type has already been
  /// transformed if it is NULL or if it is not dependent.
  bool AlreadyTransformed(QualType T);

  /// \brief Returns the location of the entity being instantiated, if known.
  SourceLocation getBaseLocation() { return Loc; }

  /// \brief Returns the name of the entity being instantiated, if any.
  DeclarationName getBaseEntity() { return Entity; }

  /// \brief Sets the "base" location and entity when that
  /// information is known based on another transformation.
  void setBase(SourceLocation Loc, DeclarationName Entity) {
    this->Loc = Loc;
    this->Entity = Entity;
  }

  bool TryExpandParameterPacks(SourceLocation EllipsisLoc,
    SourceRange PatternRange,
    llvm::ArrayRef<UnexpandedParameterPack> Unexpanded,
    bool &ShouldExpand,
    bool &RetainExpansion,
    llvm::Optional<unsigned> &NumExpansions) {
      return getSema().CheckParameterPacksForExpansion(EllipsisLoc, 
        PatternRange, Unexpanded,
        TemplateArgs, 
        ShouldExpand,
        RetainExpansion,
        NumExpansions);
  }

  void ExpandingFunctionParameterPack(ParmVarDecl *Pack) { 
    SemaRef.CurrentInstantiationScope->MakeInstantiatedLocalArgPack(Pack);
  }

  TemplateArgument ForgetPartiallySubstitutedPack();

  void RememberPartiallySubstitutedPack(TemplateArgument Arg);

  /// \brief Transform the given declaration by instantiating a reference to
  /// this declaration.
  Decl *TransformDecl(SourceLocation Loc, Decl *D);

  void transformAttrs(Decl *Old, Decl *New) { 
    SemaRef.InstantiateAttrs(TemplateArgs, Old, New);
  }

  void transformedLocalDecl(Decl *Old, Decl *New) {
    SemaRef.CurrentInstantiationScope->InstantiatedLocal(Old, New);
  }

  /// \brief Transform the definition of the given declaration by
  /// instantiating it.
  Decl *TransformDefinition(SourceLocation Loc, Decl *D);

  /// \brief Transform the first qualifier within a scope by instantiating the
  /// declaration.
  NamedDecl *TransformFirstQualifierInScope(NamedDecl *D, SourceLocation Loc);

  /// \brief Rebuild the exception declaration and register the declaration
  /// as an instantiated local.
  VarDecl *RebuildExceptionDecl(VarDecl *ExceptionDecl, 
    TypeSourceInfo *Declarator,
    SourceLocation StartLoc,
    SourceLocation NameLoc,
    IdentifierInfo *Name);

  /// \brief Rebuild the Objective-C exception declaration and register the 
  /// declaration as an instantiated local.
  VarDecl *RebuildObjCExceptionDecl(VarDecl *ExceptionDecl, 
    TypeSourceInfo *TSInfo, QualType T);

  /// \brief Check for tag mismatches when instantiating an
  /// elaborated type.
  QualType RebuildElaboratedType(SourceLocation KeywordLoc,
    ElaboratedTypeKeyword Keyword,
    NestedNameSpecifierLoc QualifierLoc,
    QualType T);

  TemplateName TransformTemplateName(CXXScopeSpec &SS,
    TemplateName Name,
    SourceLocation NameLoc,                                     
    QualType ObjectType = QualType(),
    NamedDecl *FirstQualifierInScope = 0);

  ExprResult TransformPredefinedExpr(PredefinedExpr *E);
  ExprResult TransformDeclRefExpr(DeclRefExpr *E);
  ExprResult TransformCXXDefaultArgExpr(CXXDefaultArgExpr *E);

  ExprResult TransformTemplateParmRefExpr(DeclRefExpr *E,
    NonTypeTemplateParmDecl *D);
  ExprResult TransformSubstNonTypeTemplateParmPackExpr(
    SubstNonTypeTemplateParmPackExpr *E);

  /// \brief Rebuild a DeclRefExpr for a ParmVarDecl reference.
  ExprResult RebuildParmVarDeclRefExpr(ParmVarDecl *PD, SourceLocation Loc);

  /// \brief Transform a reference to a function parameter pack.
  ExprResult TransformFunctionParmPackRefExpr(DeclRefExpr *E,
    ParmVarDecl *PD);

  /// \brief Transform a FunctionParmPackExpr which was built when we couldn't
  /// expand a function parameter pack reference which refers to an expanded
  /// pack.
  ExprResult TransformFunctionParmPackExpr(FunctionParmPackExpr *E);

  QualType TransformFunctionProtoType(TypeLocBuilder &TLB,
    FunctionProtoTypeLoc TL);
  QualType TransformFunctionProtoType(TypeLocBuilder &TLB,
    FunctionProtoTypeLoc TL,
    CXXRecordDecl *ThisContext,
    unsigned ThisTypeQuals);

  ParmVarDecl *TransformFunctionTypeParam(ParmVarDecl *OldParm,
    int indexAdjustment,
    llvm::Optional<unsigned> NumExpansions,
    bool ExpectParameterPack);

  /// \brief Transforms a template type parameter type by performing
  /// substitution of the corresponding template type argument.
  QualType TransformTemplateTypeParmType(TypeLocBuilder &TLB,
    TemplateTypeParmTypeLoc TL);

  /// \brief Transforms an already-substituted template type parameter pack
  /// into either itself (if we aren't substituting into its pack expansion)
  /// or the appropriate substituted argument.
  QualType TransformSubstTemplateTypeParmPackType(TypeLocBuilder &TLB,
    SubstTemplateTypeParmPackTypeLoc TL);

  ExprResult TransformCallExpr(CallExpr *CE) {
    getSema().CallsUndergoingInstantiation.push_back(CE);
    ExprResult Result =
      TreeTransform<TemplateInstantiator>::TransformCallExpr(CE);
    getSema().CallsUndergoingInstantiation.pop_back();
    return Result;
  }

  ExprResult TransformLambdaExpr(LambdaExpr *E) {
    LocalInstantiationScope Scope(SemaRef, /*CombineWithOuterScope=*/true);
    return TreeTransform<TemplateInstantiator>::TransformLambdaExpr(E);
  }

  ExprResult TransformLambdaScope(LambdaExpr *E,
    CXXMethodDecl *CallOperator) {
      // FVTODO: - not sure i understand why this is ...
      // If this is a generic Lambda, skip setting instantiation 
      // of Member Function
      if (!CallOperator->getDescribedFunctionTemplate())
        CallOperator->setInstantiationOfMemberFunction(E->getCallOperator(),
        TSK_ImplicitInstantiation);
      return TreeTransform<TemplateInstantiator>::
        TransformLambdaScope(E, CallOperator);
  }

private:
  ExprResult transformNonTypeTemplateParmRef(NonTypeTemplateParmDecl *parm,
    SourceLocation loc,
    TemplateArgument arg);
};
} // end namespace clang
#endif
