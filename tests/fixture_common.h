/**
 * @file    fixture_common.h
 * @brief   Shared AST-builder helpers for the hand-built fixture trees
 * @author  Vojtech Urich (xurich00)
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * Every helper mirrors what the (unfinished) parser will build: only the
 * parser-responsible fields are filled (names, node types, literal values,
 * declared types, isMain); semanticType and convertToDouble stay zero so the
 * fixture has to go through semantAnalyse() before codegen.
 *
 * Builders take ownership of every subnode argument and free the whole
 * subtree on allocation failure, so `return fbinOp(...)` chains stay
 * leak-free on abort paths.
 */

#ifndef FIXTURE_COMMON_H
#define FIXTURE_COMMON_H

#include "../ast.h"

/// @brief Copies an identifier so that deleteNode() can free() it.
char *fDupName(const char *text);

/// @brief NODE_LITERAL holding an Int constant.
ASTNode *fLitInt(int64_t value);

/// @brief NODE_LITERAL holding a Double constant.
ASTNode *fLitDouble(double value);

/// @brief NODE_LITERAL holding a String constant (escape decoding is codegen's job).
ASTNode *fLitStr(const char *value);

/// @brief NODE_VAR_REF, the parser has not resolved the name yet.
ASTNode *fVarRef(const char *name);

/// @brief NODE_BIN_OP, takes ownership of both operands.
ASTNode *fBinOp(BinaryOpType operation, ASTNode *left, ASTNode *right);

/// @brief NODE_UNARY_OP, takes ownership of the operand.
ASTNode *fUnaryOp(UnaryOpType operation, ASTNode *operand);

/// @brief NODE_FUNC_CALL, takes ownership of the argument chain (NULL = no args).
ASTNode *fCall(const char *funcName, ASTNode *args);

/// @brief Two-argument call helper: fCall(name, fArgs(a, b)).
ASTNode *fArgs2(ASTNode *first, ASTNode *second);

/// @brief Three-argument call helper: fCall(name, fArgs3(a, b, c)).
ASTNode *fArgs3(ASTNode *first, ASTNode *second, ASTNode *third);

/// @brief NODE_VAR_DECL; @p varType is TYPE_UNKNOWN when the source omits it.
ASTNode *fVarDecl(const char *name, dataType varType, bool isMutable, ASTNode *initExpr);

/// @brief NODE_ASSIGN, one statement of a reassignable variable.
ASTNode *fAssign(const char *targetName, ASTNode *expr);

/// @brief NODE_RETURN with `expr == NULL` for a bare `return` (Unit functions).
ASTNode *fReturn(ASTNode *expr);

/// @brief NODE_BLOCK wrapper around a statement chain (NULL = empty block, `()`).
ASTNode *fBlock(ASTNode *firstStmt);

/// @brief NODE_IF; @p elseBlock may be NULL (if without else).
ASTNode *fIf(ASTNode *condition, ASTNode *thenBlock, ASTNode *elseBlock);

/// @brief NODE_WHILE, takes ownership of condition and body.
ASTNode *fWhile(ASTNode *condition, ASTNode *bodyBlock);

/// @brief NODE_PARAM, one entry of a parameter list.
ASTNode *fParam(const char *name, dataType type);

/// @brief NODE_FUNC_DEF, takes ownership of the parameter list and the body.
ASTNode *fFuncDef(const char *name, dataType returnType, ASTNode *params, ASTNode *body,
                  bool isMain);

/// @brief Appends a node to the end of a sibling chain (head updated when empty).
void fChain(ASTNode **head, ASTNode *node);

#endif // FIXTURE_COMMON_H
