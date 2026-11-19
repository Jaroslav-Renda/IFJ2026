/**
 * @file    fixture_common.c
 * @brief   Shared AST-builder helpers for the hand-built fixture trees
 * @author  Vojtech Urich (xurich00)
 * @date    2026-11-19
 * @details project: IFJ2026
 */

#include "fixture_common.h"
#include <stdlib.h>
#include <string.h>

char *fDupName(const char *text) {
    size_t len = strlen(text) + 1;
    char *copy = malloc(len);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, text, len);
    return copy;
}

ASTNode *fLitInt(int64_t value) {
    ASTNode *node = createNode(NODE_LITERAL);
    if (node == NULL) {
        return NULL;
    }
    node->data.literal.litType = TYPE_INT;
    node->data.literal.value.intVal = value;
    return node;
}

ASTNode *fLitDouble(double value) {
    ASTNode *node = createNode(NODE_LITERAL);
    if (node == NULL) {
        return NULL;
    }
    node->data.literal.litType = TYPE_DOUBLE;
    node->data.literal.value.doubleVal = value;
    return node;
}

ASTNode *fLitStr(const char *value) {
    ASTNode *node = createNode(NODE_LITERAL);
    if (node == NULL) {
        return NULL;
    }
    node->data.literal.litType = TYPE_STRING;
    node->data.literal.value.strVal = fDupName(value);
    if (node->data.literal.value.strVal == NULL) {
        free(node);
        return NULL;
    }
    return node;
}

ASTNode *fVarRef(const char *name) {
    ASTNode *node = createNode(NODE_VAR_REF);
    if (node == NULL) {
        return NULL;
    }
    node->data.varRef.name = fDupName(name);
    if (node->data.varRef.name == NULL) {
        free(node);
        return NULL;
    }
    return node;
}

ASTNode *fBinOp(BinaryOpType operation, ASTNode *left, ASTNode *right) {
    ASTNode *node = createNode(NODE_BIN_OP);
    if (node == NULL) {
        deleteNode(left);
        deleteNode(right);
        return NULL;
    }
    node->data.binOp.op = operation;
    node->data.binOp.left = left;
    node->data.binOp.right = right;
    return node;
}

ASTNode *fUnaryOp(UnaryOpType operation, ASTNode *operand) {
    ASTNode *node = createNode(NODE_UNARY_OP);
    if (node == NULL) {
        deleteNode(operand);
        return NULL;
    }
    node->data.unaryOp.op = operation;
    node->data.unaryOp.operand = operand;
    return node;
}

ASTNode *fCall(const char *funcName, ASTNode *args) {
    ASTNode *node = createNode(NODE_FUNC_CALL);
    if (node == NULL) {
        deleteNode(args);
        return NULL;
    }
    node->data.funcCall.funcName = fDupName(funcName);
    node->data.funcCall.args = args;
    if (node->data.funcCall.funcName == NULL) {
        deleteNode(args);
        free(node);
        return NULL;
    }
    return node;
}

ASTNode *fArgs2(ASTNode *first, ASTNode *second) {
    first->next = second;
    return first;
}

ASTNode *fArgs3(ASTNode *first, ASTNode *second, ASTNode *third) {
    first->next = second;
    second->next = third;
    return first;
}

ASTNode *fVarDecl(const char *name, dataType varType, bool isMutable, ASTNode *initExpr) {
    ASTNode *node = createNode(NODE_VAR_DECL);
    if (node == NULL) {
        deleteNode(initExpr);
        return NULL;
    }
    node->data.varDecl.name = fDupName(name);
    node->data.varDecl.varType = varType;
    node->data.varDecl.isMutable = isMutable;
    node->data.varDecl.initExpr = initExpr;
    if (node->data.varDecl.name == NULL) {
        deleteNode(initExpr);
        free(node);
        return NULL;
    }
    return node;
}

ASTNode *fAssign(const char *targetName, ASTNode *expr) {
    ASTNode *node = createNode(NODE_ASSIGN);
    if (node == NULL) {
        deleteNode(expr);
        return NULL;
    }
    node->data.assign.targetName = fDupName(targetName);
    node->data.assign.expr = expr;
    if (node->data.assign.targetName == NULL) {
        deleteNode(expr);
        free(node);
        return NULL;
    }
    return node;
}

ASTNode *fReturn(ASTNode *expr) {
    ASTNode *node = createNode(NODE_RETURN);
    if (node == NULL) {
        deleteNode(expr);
        return NULL;
    }
    node->data.returnStmt.expr = expr;
    return node;
}

ASTNode *fBlock(ASTNode *firstStmt) {
    ASTNode *node = createNode(NODE_BLOCK);
    if (node == NULL) {
        deleteNode(firstStmt);
        return NULL;
    }
    node->data.block.firstStmt = firstStmt;
    return node;
}

ASTNode *fIf(ASTNode *condition, ASTNode *thenBlock, ASTNode *elseBlock) {
    ASTNode *node = createNode(NODE_IF);
    if (node == NULL) {
        deleteNode(condition);
        deleteNode(thenBlock);
        deleteNode(elseBlock);
        return NULL;
    }
    node->data.ifStmt.condition = condition;
    node->data.ifStmt.thenBlock = thenBlock;
    node->data.ifStmt.elseBlock = elseBlock;
    return node;
}

ASTNode *fWhile(ASTNode *condition, ASTNode *bodyBlock) {
    ASTNode *node = createNode(NODE_WHILE);
    if (node == NULL) {
        deleteNode(condition);
        deleteNode(bodyBlock);
        return NULL;
    }
    node->data.whileStmt.condition = condition;
    node->data.whileStmt.bodyBlock = bodyBlock;
    return node;
}

ASTNode *fParam(const char *name, dataType type) {
    ASTNode *node = createNode(NODE_PARAM);
    if (node == NULL) {
        return NULL;
    }
    node->data.param.name = fDupName(name);
    node->data.param.type = type;
    if (node->data.param.name == NULL) {
        free(node);
        return NULL;
    }
    return node;
}

ASTNode *fFuncDef(const char *name, dataType returnType, ASTNode *params, ASTNode *body,
                  bool isMain) {
    ASTNode *node = createNode(NODE_FUNC_DEF);
    if (node == NULL) {
        deleteNode(params);
        deleteNode(body);
        return NULL;
    }
    node->data.funcDef.name = fDupName(name);
    node->data.funcDef.returnType = returnType;
    node->data.funcDef.params = params;
    node->data.funcDef.body = body;
    node->data.funcDef.isMain = isMain;
    if (node->data.funcDef.name == NULL) {
        deleteNode(params);
        deleteNode(body);
        free(node);
        return NULL;
    }
    return node;
}

void fChain(ASTNode **head, ASTNode *node) {
    if (*head == NULL) {
        *head = node;
        return;
    }
    ASTNode *tail = *head;
    while (tail->next != NULL) {
        tail = tail->next;
    }
    tail->next = node;
}
