/**
 * @file    codegen.h
 * @brief   IFJcode26 generator module
 * @author  Petr Mácha (xmachap00)
 * @details project_IFJ2026
 */

#ifndef IFJ2026_CODEGEN_H
#define IFJ2026_CODEGEN_H

#include "ast.h"

typedef enum : uint8_t { TF, LF, GF } codegenFrameTypes;

void codegenLiteral(ASTNode *node);

void codegenFuncDef(ASTNode *node);

void codegenFuncCall(ASTNode *node);

void codegenReturn(ASTNode *node);

void codegenUnaryOp(ASTNode *node);

void codegenBinaryOp(ASTNode *node);

void codegenPostOrderExpr(ASTNode *node);

void codegenVarDecl(ASTNode *node);

void codegenParam(ASTNode *node);

void codegenFunctionDef(ASTNode *node);

void codegenWalk(ASTNode *node, bool inFuncBody);

void codegenStart(ASTNode *root);

#endif // IFJ2026_CODEGEN_H
