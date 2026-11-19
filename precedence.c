/**
 * @file    precedence.c
 * @brief   Implementation of precedence analysis (expression parsing and AST construction)
 * @author  Jaroslav Renda (xrendaj00), Jakub Poláček (xpolacj00)
 * @details project_IFJ2026
 */

#include "precedence.h"
#include "ast.h"
#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "precstack.h"
#include "prectable.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static bool nextToken(currentToken *cur) { return (getToken(&cur->token) != 1); }

static bool isExpressionEnd(Token *token) {
    switch (token->type) {
    case TOKEN_EOF:
    case TOKEN_EOL:
    case TOKEN_SEMICOLON:
        return true;
    case TOKEN_KEYWORD:
        switch (token->data.keywordVal) {
        case KEYWORD_DO:
        case KEYWORD_THEN:
            return true;
        default:
            return false;
        }
    default:
        return false;
    }
}

static bool isUnaryMinus(PrecStack *stack) {
    if (stack == NULL || stack->top == NULL) {
        errorInternal(0, "stack or stack->top is NULL");
        return false;
    }
    switch (stack->top->type) {
    case P_STACK_ID:
    case P_STACK_EXP:
    case P_STACK_LIT:
    case P_STACK_RPAR:
        return false;
    default:
        return true;
    }
}

static bool translateTokenToStack(Token *token, PrecStackType *type, bool isNeg) {
    if (token == NULL) {
        errorInternal(0, "NULL argument");
        return false;
    }
    switch (token->type) {
    case TOKEN_IDENTIFIER:
        *type = P_STACK_ID;
        return true;
    case TOKEN_MINUS:
        *type = isNeg ? P_STACK_NEG : P_STACK_MIN;
        return true;
    case TOKEN_DIVIDE:
        *type = P_STACK_DIV;
        return true;
    case TOKEN_MULTIPLY:
        *type = P_STACK_TIME;
        return true;
    case TOKEN_PLUS:
        *type = P_STACK_PLS;
        return true;
    case TOKEN_COMPARE_SMALLER_THAN:
        *type = P_STACK_LT;
        return true;
    case TOKEN_COMPARE_LESSER_EQUAL:
        *type = P_STACK_LET;
        return true;
    case TOKEN_COMPARE_BIGGER_EQUAL:
        *type = P_STACK_GET;
        return true;
    case TOKEN_COMPARE_BIGGER_THAN:
        *type = P_STACK_GT;
        return true;
    case TOKEN_EQUAL_TO:
        *type = P_STACK_EQ;
        return true;
    case TOKEN_NOT_EQUAL_TO:
        *type = P_STACK_NEQ;
        return true;
    case TOKEN_LEFT_BRACKET:
        *type = P_STACK_LPAR;
        return true;
    case TOKEN_RIGHT_BRACKET:
        *type = P_STACK_RPAR;
        return true;
    case TOKEN_COMMA:
        *type = P_STACK_COM;
        return true;
    case TOKEN_STRING:
    case TOKEN_INT:
    case TOKEN_DOUBLE:
        *type = P_STACK_LIT;
        return true;
    default:
        errorParser(token->line, "unexpected token in expression");
        return false;
    }
}

static bool translateStacktoTable(PrecStackElem *stackEl, precOperators *tableOp) {
    if (stackEl == NULL) {
        errorInternal(0, "NULL argument");
        return false;
    }
    switch (stackEl->type) {
    case P_STACK_ID:
        *tableOp = P_ID;
        return true;
    case P_STACK_NEG:
        *tableOp = P_NEG;
        return true;
    case P_STACK_DIV:
        *tableOp = P_DIV;
        return true;
    case P_STACK_TIME:
        *tableOp = P_TIME;
        return true;
    case P_STACK_MIN:
        *tableOp = P_MIN;
        return true;
    case P_STACK_PLS:
        *tableOp = P_PLS;
        return true;
    case P_STACK_LT:
        *tableOp = P_LT;
        return true;
    case P_STACK_LET:
        *tableOp = P_LET;
        return true;
    case P_STACK_GET:
        *tableOp = P_GET;
        return true;
    case P_STACK_GT:
        *tableOp = P_GT;
        return true;
    case P_STACK_EQ:
        *tableOp = P_EQ;
        return true;
    case P_STACK_NEQ:
        *tableOp = P_NEQ;
        return true;
    case P_STACK_LPAR:
        *tableOp = P_LPAR;
        return true;
    case P_STACK_RPAR:
        *tableOp = P_RPAR;
        return true;
    case P_STACK_COM:
        *tableOp = P_COM;
        return true;
    case P_STACK_LIT:
        *tableOp = P_LIT;
        return true;
    case P_STACK_END:
        *tableOp = P_END;
        return true;
    default:
        errorInternal(0, "invalid stack terminal type");
        return false;
    }
}

static bool translateTokenToTable(Token *token, precOperators *tableOp, bool isNeg) {
    if (token == NULL) {
        errorInternal(0, "NULL argument");
        return false;
    }
    switch (token->type) {
    case TOKEN_IDENTIFIER:
        *tableOp = P_ID;
        return true;
    case TOKEN_MINUS:
        *tableOp = isNeg ? P_NEG : P_MIN;
        return true;
    case TOKEN_DIVIDE:
        *tableOp = P_DIV;
        return true;
    case TOKEN_MULTIPLY:
        *tableOp = P_TIME;
        return true;
    case TOKEN_PLUS:
        *tableOp = P_PLS;
        return true;
    case TOKEN_COMPARE_SMALLER_THAN:
        *tableOp = P_LT;
        return true;
    case TOKEN_COMPARE_LESSER_EQUAL:
        *tableOp = P_LET;
        return true;
    case TOKEN_COMPARE_BIGGER_EQUAL:
        *tableOp = P_GET;
        return true;
    case TOKEN_COMPARE_BIGGER_THAN:
        *tableOp = P_GT;
        return true;
    case TOKEN_EQUAL_TO:
        *tableOp = P_EQ;
        return true;
    case TOKEN_NOT_EQUAL_TO:
        *tableOp = P_NEQ;
        return true;
    case TOKEN_LEFT_BRACKET:
        *tableOp = P_LPAR;
        return true;
    case TOKEN_RIGHT_BRACKET:
        *tableOp = P_RPAR;
        return true;
    case TOKEN_COMMA:
        *tableOp = P_COM;
        return true;
    case TOKEN_STRING:
    case TOKEN_INT:
    case TOKEN_DOUBLE:
        *tableOp = P_LIT;
        return true;
    default:
        errorParser(token->line, "unexpected token in expression");
        return false;
    }
}

/**
 * @brief Maps a binary operator stack symbol to the corresponding AST BinaryOpType.
 */
static bool translateStackOpToBinOp(PrecStackType stackOp, BinaryOpType *outOp) {
    switch (stackOp) {
    case P_STACK_PLS:
        *outOp = OP_ADD;
        return true;
    case P_STACK_MIN:
        *outOp = OP_SUB;
        return true;
    case P_STACK_TIME:
        *outOp = OP_MUL;
        return true;
    case P_STACK_DIV:
        *outOp = OP_DIV;
        return true;
    case P_STACK_EQ:
        *outOp = OP_EQ;
        return true;
    case P_STACK_NEQ:
        *outOp = OP_NEQ;
        return true;
    case P_STACK_LT:
        *outOp = OP_LT;
        return true;
    case P_STACK_GT:
        *outOp = OP_GT;
        return true;
    case P_STACK_LET:
        *outOp = OP_LTE;
        return true;
    case P_STACK_GET:
        *outOp = OP_GTE;
        return true;
    default:
        return false;
    }
}

/**
 * @brief Creates a leaf AST node (NODE_VAR_REF or NODE_LITERAL) when shifting an operand token.
 *        Transfers ownership of heap-allocated strings from the token to the AST node.
 */
static bool attachLeafNodeFromToken(PrecStackElem *elem, Token *token) {
    switch (token->type) {
    case TOKEN_IDENTIFIER: {
        ASTNode *node = createNode(NODE_VAR_REF);
        if (node == NULL) {
            errorInternal(token->line, "Memory allocation failed for NODE_VAR_REF");
            return false;
        }
        node->data.varRef.name = token->data.identifier;
        token->data.identifier = NULL;
        elem->node = node;
        return true;
    }
    case TOKEN_INT: {
        ASTNode *node = createNode(NODE_LITERAL);
        if (node == NULL) {
            errorInternal(token->line, "Memory allocation failed for NODE_LITERAL (int)");
            return false;
        }
        node->data.literal.litType = TYPE_INT;
        // TODO: zkontroluj název pole pro int v lexer.h (např. intVal)
        node->data.literal.value.intVal = token->data.intVal;
        elem->node = node;
        return true;
    }
    case TOKEN_DOUBLE: {
        ASTNode *node = createNode(NODE_LITERAL);
        if (node == NULL) {
            errorInternal(token->line, "Memory allocation failed for NODE_LITERAL (double)");
            return false;
        }
        node->data.literal.litType = TYPE_DOUBLE;
        // TODO: zkontroluj název pole pro double v lexer.h (např. doubleVal)
        node->data.literal.value.doubleVal = token->data.doubleVal;
        elem->node = node;
        return true;
    }
    case TOKEN_STRING: {
        ASTNode *node = createNode(NODE_LITERAL);
        if (node == NULL) {
            errorInternal(token->line, "Memory allocation failed for NODE_LITERAL (string)");
            return false;
        }
        node->data.literal.litType = TYPE_STRING;
        // TODO: zkontroluj název pole pro string v lexer.h (např. stringVal / strVal)
        node->data.literal.value.strVal = token->data.stringVal;
        token->data.stringVal = NULL;
        elem->node = node;
        return true;
    }
    default:
        elem->node = NULL;
        return true;
    }
}

static bool shift(PrecStack *stack, Token *token, bool isNeg) {
    PrecStackType type;
    if (!translateTokenToStack(token, &type, isNeg)) {
        return false;
    }

    if (!precPushAfterTerminal(stack, P_STACK_STOP)) {
        return false;
    }
    if (!precPush(stack, type)) {
        return false;
    }

    return attachLeafNodeFromToken(stack->top, token);
}

static bool shiftNoStop(PrecStack *stack, Token *token, bool isNeg) {
    PrecStackType type;
    if (!translateTokenToStack(token, &type, isNeg)) {
        return false;
    }

    if (!precPush(stack, type)) {
        return false;
    }

    return attachLeafNodeFromToken(stack->top, token);
}

/**
 * @brief Pops `count` symbols plus the P_STACK_STOP marker and pushes P_STACK_EXP with `newNode`.
 *        Callers must extract/detach child ASTNodes from stack elements before calling this.
 */
static bool reduceNEl(PrecStack *stack, uint32_t count, ASTNode *newNode, bool isFooCall) {
    if (newNode == NULL) {
        errorInternal(0, "NULL ASTNode passed to reduceNEl");
        return false;
    }

    for (uint32_t i = 0; i < count; i++) {
        if (stack->top == NULL) {
            deleteNode(newNode);
            errorParser(0, "unexpected empty stack during reduction");
            return false;
        }
        if (stack->top->node != NULL) {
            deleteNode(stack->top->node);
            stack->top->node = NULL;
        }
        if (!precPop(stack)) {
            deleteNode(newNode);
            return false;
        }
    }

    if (stack->top == NULL || stack->top->type != P_STACK_STOP) {
        deleteNode(newNode);
        errorParser(0, "missing '<' stop marker on stack");
        return false;
    }
    if (!precPop(stack)) {
        deleteNode(newNode);
        return false;
    }

    if (!precPush(stack, P_STACK_EXP)) {
        deleteNode(newNode);
        return false;
    }

    stack->top->node = newNode;
    if (isFooCall) {
        stack->top->isFooCall = true;
    }
    return true;
}

static bool reduceRparStart(PrecStackElem *elem, PrecStack *stack) {
    if (elem == NULL) {
        errorParser(0, "unexpected end of stack before ')'");
        return false;
    }

    switch (elem->type) {
    // E -> id()
    case P_STACK_LPAR: {
        PrecStackElem *idElem = elem->next;
        if (idElem == NULL) {
            errorParser(0, "empty parentheses '()' require a function identifier");
            return false;
        }
        if (idElem->type != P_STACK_ID || idElem->node == NULL) {
            errorParser(0, "expected function identifier before '()'");
            return false;
        }

        ASTNode *callNode = createNode(NODE_FUNC_CALL);
        if (callNode == NULL) {
            errorInternal(0, "Memory allocation failed for NODE_FUNC_CALL");
            return false;
        }
        callNode->data.funcCall.funcName = idElem->node->data.varRef.name;
        idElem->node->data.varRef.name = NULL;
        callNode->data.funcCall.args = NULL;

        // Free the temporary NODE_VAR_REF wrapper (its name was moved to callNode)
        deleteNode(idElem->node);
        idElem->node = NULL;

        return reduceNEl(stack, 3, callNode, true);
    }

    // E -> (E) | id(E)
    case P_STACK_EXP: {
        PrecStackElem *expElem = elem;
        PrecStackElem *lparElem = expElem->next;
        if (lparElem == NULL || lparElem->type != P_STACK_LPAR) {
            errorParser(0, "missing '(' matching ')'");
            return false;
        }
        PrecStackElem *beforeLpar = lparElem->next;
        if (beforeLpar == NULL) {
            errorParser(0, "unexpected end of stack before '('");
            return false;
        }

        // E -> id(E)
        if (beforeLpar->type == P_STACK_ID) {
            if (beforeLpar->node == NULL || expElem->node == NULL) {
                errorInternal(0, "missing AST node in function call reduction");
                return false;
            }
            ASTNode *callNode = createNode(NODE_FUNC_CALL);
            if (callNode == NULL) {
                errorInternal(0, "Memory allocation failed for NODE_FUNC_CALL");
                return false;
            }
            callNode->data.funcCall.funcName = beforeLpar->node->data.varRef.name;
            beforeLpar->node->data.varRef.name = NULL;
            callNode->data.funcCall.args = expElem->node;

            deleteNode(beforeLpar->node);
            beforeLpar->node = NULL;
            expElem->node = NULL;

            return reduceNEl(stack, 4, callNode, true);
        }

        // E -> (E)
        if (beforeLpar->type == P_STACK_STOP) {
            if (expElem->node == NULL || expElem->node->next != NULL) {
                errorParser(
                    0, "comma-separated expressions are only allowed as function arguments");
                return false;
            }
            ASTNode *innerExpr = expElem->node;
            expElem->node = NULL;
            return reduceNEl(stack, 3, innerExpr, false);
        }

        errorParser(0, "unexpected symbol before '('");
        return false;
    }

    default:
        errorParser(0, "unexpected symbol before ')'");
        return false;
    }
}

static bool reduceExpStart(PrecStackElem *opElem, PrecStack *stack) {
    if (opElem == NULL) {
        errorParser(0, "unexpected end of stack during expression reduction");
        return false;
    }

    PrecStackElem *rightElem = stack->top;

    switch (opElem->type) {
    // E -> -E
    case P_STACK_NEG: {
        if (rightElem->node == NULL || rightElem->node->next != NULL) {
            errorParser(0, "invalid operand for unary minus");
            return false;
        }
        ASTNode *unaryNode = createNode(NODE_UNARY_OP);
        if (unaryNode == NULL) {
            errorInternal(0, "Memory allocation failed for NODE_UNARY_OP");
            return false;
        }
        unaryNode->data.unaryOp.op = UOP_MINUS;
        unaryNode->data.unaryOp.operand = rightElem->node;
        rightElem->node = NULL;

        return reduceNEl(stack, 2, unaryNode, false);
    }

    // E -> E , E (chains function call arguments via ->next in left-to-right order)
    case P_STACK_COM: {
        PrecStackElem *leftElem = opElem->next;
        if (leftElem == NULL || leftElem->type != P_STACK_EXP) {
            errorParser(0, "missing left expression before ','");
            return false;
        }
        if (leftElem->node == NULL || rightElem->node == NULL) {
            errorInternal(0, "missing AST node in argument list reduction");
            return false;
        }

        ASTNode *argListHead = leftElem->node;
        ASTNode *tail = argListHead;
        while (tail->next != NULL) {
            tail = tail->next;
        }
        tail->next = rightElem->node;

        leftElem->node = NULL;
        rightElem->node = NULL;
        return reduceNEl(stack, 3, argListHead, false);
    }

    // E -> E op E
    case P_STACK_DIV:
    case P_STACK_TIME:
    case P_STACK_MIN:
    case P_STACK_PLS:
    case P_STACK_LT:
    case P_STACK_LET:
    case P_STACK_GET:
    case P_STACK_GT:
    case P_STACK_EQ:
    case P_STACK_NEQ: {
        PrecStackElem *leftElem = opElem->next;
        if (leftElem == NULL || leftElem->type != P_STACK_EXP) {
            errorParser(0, "missing left operand for binary operator");
            return false;
        }
        if (leftElem->node == NULL || rightElem->node == NULL || leftElem->node->next != NULL ||
            rightElem->node->next != NULL) {
            errorParser(0, "invalid operand for binary operator");
            return false;
        }

        BinaryOpType binOp;
        if (!translateStackOpToBinOp(opElem->type, &binOp)) {
            errorInternal(0, "unknown binary operator on precedence stack");
            return false;
        }

        ASTNode *binNode = createNode(NODE_BIN_OP);
        if (binNode == NULL) {
            errorInternal(0, "Memory allocation failed for NODE_BIN_OP");
            return false;
        }
        binNode->data.binOp.op = binOp;
        binNode->data.binOp.left = leftElem->node;
        binNode->data.binOp.right = rightElem->node;

        leftElem->node = NULL;
        rightElem->node = NULL;
        return reduceNEl(stack, 3, binNode, false);
    }

    default:
        errorParser(0, "unexpected operator during expression reduction");
        return false;
    }
}

static bool reduce(PrecStack *stack) {
    PrecStackElem *elem = stack->top;
    if (elem == NULL) {
        errorParser(0, "empty stack during reduction");
        return false;
    }

    switch (elem->type) {
    // E -> id | literal
    case P_STACK_ID:
    case P_STACK_LIT: {
        ASTNode *leafNode = elem->node;
        if (leafNode == NULL) {
            errorInternal(0, "missing leaf ASTNode on stack");
            return false;
        }
        elem->node = NULL;
        return reduceNEl(stack, 1, leafNode, false);
    }

    // E -> -E | E op E | E , E
    case P_STACK_EXP:
        elem = elem->next;
        if (elem == NULL) {
            errorParser(0, "unexpected end of stack after expression");
            return false;
        }
        return reduceExpStart(elem, stack);

    // E -> id() | (E) | id(E)
    case P_STACK_RPAR:
        elem = elem->next;
        if (elem == NULL) {
            errorParser(0, "unexpected end of stack before ')'");
            return false;
        }
        return reduceRparStart(elem, stack);

    default:
        errorParser(0, "cannot reduce top of precedence stack");
        return false;
    }
}

static bool finishExpression(PrecStack *stack, ASTNode **outExpr) {
    PrecStackElem *elem = topTerminal(stack);
    if (elem == NULL) {
        return false;
    }

    while (elem->type != P_STACK_END) {
        if (!reduce(stack)) {
            return false;
        }

        elem = topTerminal(stack);
        if (elem == NULL) {
            return false;
        }
    }

    if (stack->top == NULL || stack->top->type != P_STACK_EXP) {
        errorParser(0, "expression did not reduce to a single E");
        return false;
    }

    if (stack->top->next == NULL || stack->top->next->type != P_STACK_END) {
        errorParser(0, "extra symbols remaining on precedence stack");
        return false;
    }

    if (stack->top->node == NULL || stack->top->node->next != NULL) {
        errorParser(0, "unexpected comma expression at top level");
        return false;
    }

    *outExpr = stack->top->node;
    stack->top->node = NULL; // Transfer ownership to caller so precDeleteStack won't free it
    return true;
}

bool resolveToken(Token *token, PrecStack *stack) {
    bool isprecPushed = false;
    bool isNeg = isUnaryMinus(stack);

    precOperators incoming;
    precOperators onStack;

    if (!translateTokenToTable(token, &incoming, isNeg)) {
        return false;
    }

    while (!isprecPushed) {
        if (!translateStacktoTable(topTerminal(stack), &onStack)) {
            return false;
        }
        precRelations result = precTable[onStack][incoming];
        switch (result) {
        case S:
            if (!shift(stack, token, isNeg)) {
                return false;
            }
            isprecPushed = true;
            break;
        case R:
            if (!reduce(stack)) {
                return false;
            }
            break;
        case E:
            errorParser(token->line, "syntax error in expression (precedence table)");
            return false;
        case Q:
            if (!shiftNoStop(stack, token, isNeg)) {
                return false;
            }
            isprecPushed = true;
            break;
        case A:
            return true;
        }
    }

    return true;
}

bool parseExpression(currentToken *cur, lookaheadToken *lat, ASTNode **outExpr) {
    *outExpr = NULL;

    PrecStack stack;
    if (!precInitStack(&stack)) {
        return false;
    }
    if (!precPush(&stack, P_STACK_END)) {
        precDeleteStack(&stack);
        return false;
    }

    // lookahead token will always be nothing or id
    if (lat != NULL && !lat->isEmpty) {
        if (!resolveToken(&lat->token, &stack)) {
            if (lat->token.type == TOKEN_IDENTIFIER && lat->token.data.identifier != NULL) {
                free(lat->token.data.identifier);
                lat->token.data.identifier = NULL;
            }
            precDeleteStack(&stack);
            return false;
        }
        lat->isEmpty = true;
    }

    // parse every other token until the end of expression
    while (!isExpressionEnd(&cur->token)) {
        if (!resolveToken(&cur->token, &stack)) {
            precDeleteStack(&stack);
            return false;
        }

        if (!nextToken(cur)) {
            precDeleteStack(&stack);
            return false;
        }
    }

    bool result = finishExpression(&stack, outExpr);
    if (!precDeleteStack(&stack)) {
        deleteNode(*outExpr);
        *outExpr = NULL;
        return false;
    }

    return result;
}
