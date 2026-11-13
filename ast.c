/**
 * @file    ast.h
 * @brief   Source file for abstract syntax tree
 * @author  Jakub Poláček (xpolacj00)
 * @date    2026-11-12
 * @details project: IFJ2026
 */
#ifndef AST_C
#define AST_C

#include "ast.h"
#include "symtable.h"
#include <stdlib.h>

ASTNode *createTree() {
    ASTNode *new = calloc(1, sizeof(ASTNode));
    if (new == NULL) {
        return NULL;
    }
    new->type = NODE_PROGRAM;
    new->data.program.funcCount = 0;
    return new;
}

ASTNode *createNode(ASTNodeType type) {
    ASTNode *new = calloc(1, sizeof(ASTNode));
    if (new == NULL) {
        return NULL;
    }
    new->type = type;
    return new;
}

void deleteNode(ASTNode *node) {
    // while cycle goes through all next-> nodes, also stops all NULL nodes so we dont have to test
    // it in switch
    while (node != NULL) {
        ASTNode *nextSibling = node->next;
        // for each case we recursivly call this func to delete nodes stored in the union
        switch (node->type) {
        case NODE_PROGRAM:
            deleteNode(node->data.program.functions);
            break;
        case NODE_BLOCK:
            deleteNode(node->data.block.firstStmt);
            break;
        case NODE_FUNC_DEF:
            if (node->data.funcDef.name != NULL) {
                free(node->data.funcDef.name);
            }
            deleteNode(node->data.funcDef.params);
            deleteNode(node->data.funcDef.body);
            break;
        case NODE_PARAM:
            if (node->data.param.name != NULL) {
                free(node->data.param.name);
            }
            break;
        case NODE_VAR_DECL:
            if (node->data.varDecl.name != NULL) {
                free(node->data.varDecl.name);
            }
            deleteNode(node->data.varDecl.initExpr);
            break;
        case NODE_ASSIGN:
            if (node->data.assign.targetName != NULL) {
                free(node->data.assign.targetName);
            }
            deleteNode(node->data.assign.expr);
            break;
        case NODE_IF:
            deleteNode(node->data.ifStmt.condition);
            deleteNode(node->data.ifStmt.thenBlock);
            deleteNode(node->data.ifStmt.elseBlock);
            break;
        case NODE_WHILE:
            deleteNode(node->data.whileStmt.condition);
            deleteNode(node->data.whileStmt.bodyBlock);
            break;
        case NODE_RETURN:
            deleteNode(node->data.returnStmt.expr);
            break;
        case NODE_BIN_OP:
            deleteNode(node->data.binOp.left);
            deleteNode(node->data.binOp.right);
            break;
        case NODE_UNARY_OP:
            deleteNode(node->data.unaryOp.operand);
            break;
        case NODE_FUNC_CALL:
            if (node->data.funcCall.funcName != NULL) {
                free(node->data.funcCall.funcName);
            }
            deleteNode(node->data.funcCall.args);
            break;
        case NODE_VAR_REF:
            if (node->data.varRef.name != NULL) {
                free(node->data.varRef.name);
            }
            break;
        case NODE_LITERAL:
            if (node->data.literal.litType == TYPE_STRING &&
                node->data.literal.value.strVal != NULL) {
                free(node->data.literal.value.strVal);
            }
            break;
        }
        free(node);
        node = nextSibling;
    }
}
#endif // AST_C
