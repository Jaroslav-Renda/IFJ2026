/**
 * @file    precstack.h
 * @brief   Header file for precedence analysis stack
 * @author  Jaroslav Renda (xrendaj00)
 * @details project: IFJ2026
 */
#ifndef PRECSTACK_H
#define PRECSTACK_H

#include "ast.h"
#include "symtable.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum : uint8_t {
    P_STACK_ID,   // id
    P_STACK_NEG,  // -E
    P_STACK_DIV,  // /
    P_STACK_TIME, // *
    P_STACK_MIN,  // -
    P_STACK_PLS,  // +
    P_STACK_LT,   // <
    P_STACK_LET,  // <=
    P_STACK_GET,  // >=
    P_STACK_GT,   // >
    P_STACK_EQ,   // ==
    P_STACK_NEQ,  // !=
    P_STACK_LPAR, // (
    P_STACK_RPAR, // )
    P_STACK_COM,  // ,
    P_STACK_LIT,  // int, double, string
    P_STACK_END,  // $
    P_STACK_EXP,  // E (represents reduced expression)
    P_STACK_STOP, // stop elemnt inserted with shift
} PrecStackType;

typedef struct PrecStackElemT {
    PrecStackType type;
    bool isFooCall;
    ASTNode *node;
    dataType litType;
    union {
        int intVal;
        double doubleVal;
        char *stringVal;
        char *identifier;
    };
    struct PrecStackElemT *next;
} PrecStackElem;

typedef struct PrecStackT {
    PrecStackElem *top;
} PrecStack;

bool precInitStack(PrecStack *stack);

bool precPush(PrecStack *stack, PrecStackType type);

bool precPushAfterTerminal(PrecStack *stack, PrecStackType type);

bool precPop(PrecStack *stack);

PrecStackElem *topTerminal(PrecStack *stack);

bool precDeleteStack(PrecStack *stack);

#endif // PRECSTACK_H
