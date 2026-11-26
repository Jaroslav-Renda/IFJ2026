/**
 * @file    precedence.c
 * @brief   Implementation of precedence analysis (expression parsing)
 * @author  Jaroslav Renda (xrendaj00)
 * @details project_IFJ2026
 */

#include "precedence.h"
#include "ast.h"
#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "precstack.h"
#include "prectable.h"
#include "symtable.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static bool nextToken(currentToken *cur) { return (getToken(&cur->token) == OK); }

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
        if (isNeg) {
            *type = P_STACK_NEG;
        } else {
            *type = P_STACK_MIN;
        }
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
        errorParser(0, "unexpected token in expression");
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
        if (isNeg) {
            *tableOp = P_NEG;
            return true;
        } else {
            *tableOp = P_MIN;
            return true;
        }
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
        errorParser(0, "unexpected token in expression");
        return false;
    }
}

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

static bool shift(PrecStack *stack, Token *token, bool isNeg) {
    PrecStackType type;
    if (!translateTokenToStack(token, &type, isNeg)) {
        return false;
    }

    // precPush <
    if (!precPushAfterTerminal(stack, P_STACK_STOP)) {
        return false;
    }
    if (!precPush(stack, type)) {
        return false;
    }

    switch (stack->top->type) {
    case P_STACK_ID:
        stack->top->identifier = token->data.identifier;
        token->data.identifier = NULL;
        break;
    case P_STACK_LIT:
        switch (token->type) {
        case TOKEN_INT:
            stack->top->litType = TYPE_INT;
            stack->top->intVal = token->data.intVal;
            break;
        case TOKEN_DOUBLE:
            stack->top->litType = TYPE_DOUBLE;
            stack->top->doubleVal = token->data.doubleVal;
            break;
        case TOKEN_STRING:
            stack->top->litType = TYPE_STRING;
            stack->top->stringVal = token->data.stringVal;
            token->data.stringVal = NULL;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return true;
}

static bool shiftNoStop(PrecStack *stack, Token *token, bool isNeg) {
    PrecStackType type;
    if (!translateTokenToStack(token, &type, isNeg)) {
        return false;
    }

    // all precPushes probably need checks (update precstack)
    if (!precPush(stack, type)) {
        return false;
    }

    return true;
}

static bool reduceNEl(PrecStack *stack, uint32_t count, bool isFooCall, ASTNode *node) {
    for (uint32_t i = 0; i < count; i++) {
        if (!precPop(stack)) {
            deleteNode(node);
            return false;
        }
    }

    if (stack->top == NULL || stack->top->type != P_STACK_STOP) {
        errorParser(0, "missing '<' stop marker on stack");
        deleteNode(node);
        return false;
    }
    if (!precPop(stack)) {
        deleteNode(node);
        return false;
    }

    if (!precPush(stack, P_STACK_EXP)) {
        deleteNode(node);
        return false;
    }

    stack->top->node = node;
    stack->top->isFooCall = isFooCall;
    return true;
}

static bool reduceId(PrecStack *stack) {
    switch (stack->top->type) {
    case P_STACK_ID: {
        ASTNode *tmpNode = createNode(NODE_VAR_REF);
        if (tmpNode == NULL) {
            errorInternal(0, "mem allocation for NODE_VAR_REF failed");
            return false;
        }
        tmpNode->data.varRef.name = stack->top->identifier;
        stack->top->identifier = NULL;

        return reduceNEl(stack, 1, false, tmpNode);
    }
    case P_STACK_LIT: {
        ASTNode *tmpNode = createNode(NODE_LITERAL);
        if (tmpNode == NULL) {
            errorInternal(0, "mem allocation for NODE_LITERAL failed");
            return false;
        }
        tmpNode->data.literal.litType = stack->top->litType;
        if (stack->top->litType == TYPE_STRING) {
            tmpNode->data.literal.value.strVal = stack->top->stringVal;
            stack->top->stringVal = NULL;
        }
        if (stack->top->litType == TYPE_INT) {
            tmpNode->data.literal.value.intVal = stack->top->intVal;
        }
        if (stack->top->litType == TYPE_DOUBLE) {
            tmpNode->data.literal.value.doubleVal = stack->top->doubleVal;
        }
        return reduceNEl(stack, 1, false, tmpNode);
    }
    default:
        return false;
    }
}

static bool reduceExpAfterPar(PrecStackElem *elem, PrecStack *stack) {
    PrecStackElem *elemPar = elem->next;
    if (elemPar == NULL || elemPar->type != P_STACK_LPAR) {
        errorParser(0, "missing '(' matching ')'");
        return false;
    }

    PrecStackElem *elemId = elemPar->next;
    if (elemId == NULL) {
        errorParser(0, "unexpected end of stack before '('");
        return false;
    }

    // E -> id(E)
    if (elemId->type == P_STACK_ID) {
        if (elemId->identifier == NULL) {
            errorParser(0, "the identifier holds no value");
            return false;
        }
        ASTNode *node = createNode(NODE_FUNC_CALL);
        if (node == NULL) {
            errorInternal(0, "mem allocation for NODE_FUNC_CALL failed");
            return false;
        }
        node->data.funcCall.funcName = elemId->identifier;
        elemId->identifier = NULL;

        node->data.funcCall.args = elem->node;
        elem->node = NULL;

        return reduceNEl(stack, 4, true, node);
    }

    // E -> (E)
    if (elemId->type == P_STACK_STOP) {
        if (elem->node == NULL || elem->node->next != NULL) {
            errorParser(0, "comma-separated expressions are only allowed as function arguments");
            return false;
        }
        ASTNode *expr = elem->node;
        elem->node = NULL;
        return reduceNEl(stack, 3, false, expr);
    }
    errorParser(0, "unexpected symbol before '('");
    return false;
}

static bool reduceRparStart(PrecStackElem *elem, PrecStack *stack) {
    if (elem == NULL) {
        errorParser(0, "unexpected end of stack before ')'");
        return false;
    }

    switch (elem->type) {

    // E -> id() || ()
    case P_STACK_LPAR: {
        // is empty foo
        elem = elem->next; // element with the id
        if (elem == NULL) {
            errorParser(0, "empty parentheses '()' require a function identifier");
            return false;
        }

        // E -> id()
        if (elem->type == P_STACK_ID) {
            if (elem->identifier == NULL) {
                errorParser(0, "identifier is NULL");
                return false;
            }

            ASTNode *node = createNode(NODE_FUNC_CALL);
            if (node == NULL) {
                errorInternal(0, "mem allocation for NODE_FUNC_CALL failed");
                return false;
            }
            node->data.funcCall.funcName = elem->identifier;
            node->data.funcCall.args = NULL;

            elem->identifier = NULL;
            return reduceNEl(stack, 3, true, node);
        }

        // E -> ()
        if (elem->type == P_STACK_STOP) {
            ASTNode *node = createNode(NODE_LITERAL);
            if (node == NULL) {
                errorInternal(0, "mem allocation for Unit literal failed");
                return false;
            }
            node->data.literal.litType = TYPE_UNIT;

            return reduceNEl(stack, 2, false, node);
        }

        errorParser(0, "unexpected symbol before '()'");
        return false;
    }

    // E -> (E) | id(E)
    case P_STACK_EXP: {
        return reduceExpAfterPar(elem, stack);
    }

    default:
        errorParser(0, "unexpected symbol before ')'");
        return false;
    }
}

static bool reduceExpStart(PrecStackElem *elem, PrecStack *stack) {
    if (elem == NULL) {
        errorParser(0, "unexpected end of stack during expression reduction");
        return false;
    }

    // right side of the operator
    PrecStackElem *rightElem = stack->top;

    switch (elem->type) {
    // E -> -E
    case P_STACK_NEG: {
        if (rightElem->node == NULL || rightElem->node->next != NULL) {
            errorParser(0, "invalid operand for unary minus");
            return false;
        }

        ASTNode *node = createNode(NODE_UNARY_OP);
        if (node == NULL) {
            errorInternal(0, "mem allocation for NODE_UNARY_OP failed");
            return false;
        }
        node->data.unaryOp.op = UOP_MINUS;
        node->data.unaryOp.operand = rightElem->node;
        rightElem->node = NULL;
        return reduceNEl(stack, 2, false, node);
    }
    // E -> E , E
    case P_STACK_COM: {
        PrecStackElem *leftElem = elem->next;

        if (leftElem == NULL || leftElem->type != P_STACK_EXP) {
            errorParser(0, "missing left expression before ','");
            return false;
        }

        if (leftElem->node == NULL || rightElem->node == NULL) {
            errorParser(0, "missing AST node in argument list reduction");
            return false;
        }

        ASTNode *head = leftElem->node;
        ASTNode *tail = head;
        while (tail->next != NULL) {
            tail = tail->next;
        }
        tail->next = rightElem->node;

        leftElem->node = NULL;
        rightElem->node = NULL;
        return reduceNEl(stack, 3, false, head);
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
        PrecStackElem *leftElem = elem->next;

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
        if (!translateStackOpToBinOp(elem->type, &binOp)) {
            errorInternal(0, "unknown binary operator on precedence stack");
            return false;
        }

        ASTNode *node = createNode(NODE_BIN_OP);
        if (node == NULL) {
            errorInternal(0, "mem allocation for NODE_BINARY_OP failed");
            return false;
        }
        node->data.binOp.op = binOp;
        node->data.binOp.left = leftElem->node;
        node->data.binOp.right = rightElem->node;

        leftElem->node = NULL;
        rightElem->node = NULL;
        return reduceNEl(stack, 3, false, node);
    }
    default:
        errorParser(0, "invalid operator or symbol before expression");
        // error
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
    case P_STACK_LIT:
        return reduceId(stack);

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

bool finishExpression(PrecStack *stack, ASTNode **expr) {
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

    // on the stack is only E so the subtree is complete, can link
    *expr = stack->top->node;
    stack->top->node = NULL;

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
                // internal error
                return false;
            }
            isprecPushed = true;
            break;
        case R:
            if (!reduce(stack)) {
                // syntax error
                return false;
            }
            break;
        case E:
            errorParser(0, "precedence analysis error TODO");
            // syntax error
            return false;
        case Q:
            if (!shiftNoStop(stack, token, isNeg)) {
                // internal error
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

// lookahead token is one token in history
bool parseExpression(currentToken *cur, lookaheadToken *lat, ASTNode **expr) {
    *expr = NULL;
    PrecStack stack;
    if (!precInitStack(&stack)) {
        return false;
    }
    if (!precPush(&stack, P_STACK_END)) { // the first token on the stack is an end '$'
        return false;
    }

    // lookahead token will always be nothing or id
    if (lat != NULL && !lat->isEmpty) {
        if (!resolveToken(&lat->token, &stack)) {
            if (lat->token.type == TOKEN_IDENTIFIER && lat->token.data.identifier != NULL) {
                free(lat->token.data.identifier);
                lat->token.data.identifier = NULL;
            }
            if (!precDeleteStack(&stack)) {
                return false;
            }
            return false;
        }
        lat->isEmpty = true;
    }

    // parse every other token until the end of expression
    while (!isExpressionEnd(&cur->token)) {
        if (!resolveToken(&cur->token, &stack)) {

            if (!precDeleteStack(&stack)) {
                return false;
            }
            return false;
        }

        if (!nextToken(cur)) {
            if (!precDeleteStack(&stack)) {
                return false;
            }
            return false;
        }
    }

    // at this point on the stack are either sub trees or expression waiting to be resolved
    // so we can resolve them and link the final expression to the node
    bool result = finishExpression(&stack, expr);
    if (!precDeleteStack(&stack)) {
        deleteNode(*expr);
        *expr = NULL;
        return false;
    }

    return result;
}
