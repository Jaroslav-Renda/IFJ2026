/**
 * @file    precedence.c
 * @brief   Implementation of precedence analysis (expression parsing)
 * @author  Jaroslav Renda (xrendaj00)
 * @details project_IFJ2026
 */

#include "precedence.h"
#include "lexer.h"
#include "precstack.h"
#include "parser.h"
#include "prectable.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "error.h"

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

static bool reduceNEl(PrecStack *stack, uint32_t count, bool isFooCall) {
    // TODO construct AST NODES
    for (uint32_t i = 0; i < count; i++) {
        if (!precPop(stack)) {
            return false;
        }
    }

    if (stack->top == NULL || stack->top->type != P_STACK_STOP) {
        errorParser(0, "missing '<' stop marker on stack");
        return false;
    }
    if (!precPop(stack)) {
        return false;
    }

    if (!precPush(stack, P_STACK_EXP)) {
        return false;
    }

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
        case P_STACK_LPAR:
            // is empty foo
            elem = elem->next;
            if (elem == NULL) {
                errorParser(0, "empty parentheses '()' require a function identifier");
                return false;
            }
            if (elem->type != P_STACK_ID) {
                errorParser(0, "expected '(' before expression");
                return false;
            }
            return reduceNEl(stack, 3, true);
        // E -> (E) | id(E)
        case P_STACK_EXP:
            elem = elem->next;
            if (elem == NULL) {
                errorParser(0, "precedence analysis error TODO");
                return false;
            }
            if (elem->type != P_STACK_LPAR) {
                errorParser(0, "precedence analysis error TODO");
                return false;
            }
            elem = elem->next;
            if (elem == NULL) {
                errorParser(0, "precedence analysis error TODO");
                return false;
            }
            // E -> id(E)
            if (elem->type == P_STACK_ID) {
                return reduceNEl(stack, 4, true);
            }
            // E -> (E)
            if (elem->type == P_STACK_STOP) {
                return reduceNEl(stack, 3, false);
            }
            errorParser(0, "precedence analysis error TODO");
            return false;

        default:
            errorParser(0, "precedence analysis error TODO");
            return false;
    }
}

static bool reduceExpStart(PrecStackElem *elem, PrecStack *stack) {
    if (elem == NULL) {
        errorParser(0, "precedence analysis error TODO");
        return false;
    }

    switch (elem->type) {
        // E -> -E
        case P_STACK_NEG:
            return reduceNEl(stack, 2, false);
        // E -> E op E | E , E
        case P_STACK_DIV:
        case P_STACK_TIME:
        case P_STACK_MIN:
        case P_STACK_PLS:
        case P_STACK_LT:
        case P_STACK_LET:
        case P_STACK_GET:
        case P_STACK_GT:
        case P_STACK_EQ:
        case P_STACK_NEQ:
        case P_STACK_COM:
            elem = elem->next;
            if (elem == NULL || elem->type != P_STACK_EXP) {
                errorParser(0, "precedence analysis error TODO");
                // error
                return false;
            }
            return reduceNEl(stack, 3, false);
        default:
            errorParser(0, "precedence analysis error TODO");
            // error
            return false;
    }
}

static bool reduce(PrecStack *stack) {
    PrecStackElem *elem = stack->top;
    if (elem == NULL) {
        errorParser(0, "precedence analysis error TODO");
        return false;
    }

    switch (elem->type) {
        // E -> id | literal
        case P_STACK_ID:
        case P_STACK_LIT:
            return reduceNEl(stack, 1, false);

        // E -> -E | E op E | E , E
        case P_STACK_EXP:
            elem = elem->next;
            if (elem == NULL) {
                errorParser(0, "precedence analysis error TODO");
                return false;
            }
            return reduceExpStart(elem, stack);

        // E -> id() | (E) | id(E)
        case P_STACK_RPAR:
            elem = elem->next;
            if (elem == NULL) {
                errorParser(0, "precedence analysis error TODO");
                return false;
            }
            return reduceRparStart(elem, stack);
        default:
            errorParser(0, "precedence analysis error TODO");
            return false;
    }
}



bool finishExpression(PrecStack *stack) {
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
        errorParser(0, "precedence analysis error TODO");
        return false;
    }

    if (stack->top->next == NULL || stack->top->next->type != P_STACK_END) {
        errorParser(0, "precedence analysis error TODO");
        return false;
    }
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
bool parseExpression(currentToken *cur, lookaheadToken *lat) {
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

    bool result = finishExpression(&stack);
    if (!precDeleteStack(&stack)) {
        return false;
    }

    return result;
}
