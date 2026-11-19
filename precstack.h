/**
 * @file    precstack.h
 * @brief   Header file for precedence analysis stack
 * @author  Jaroslav Renda (xrendaj00)
 * @details project: IFJ2026
 */
#ifndef PRECSTACK_H
#define PRECSTACK_H

#include <stdbool.h>
#include <stdint.h>

typedef enum : uint8_t {
    P_ID,   // id
    P_NEG,  // -E
    P_DIV,  // /
    P_TIME, // *
    P_MIN,  // -
    P_PLS,  // +
    P_LT,   // <
    P_LET,  // <=
    P_GET,  // >=
    P_GT,   // >
    P_EQ,   // ==
    P_NEQ,  // !=
    P_LPAR, // (
    P_RPAR, // )
    P_COM,  // ,
    P_LIT,  // int, double, string
    P_END,  // $
    P_EXP,  // E (represents reduced expression)
    P_STOP, // stop elemnt inserted with shift
} PrecStackType;

typedef struct PrecStackElemT {
    PrecStackType type;
    bool isFooCall;
    struct PrecStackElemT *next;
} PrecStackElem;

typedef struct PrecStackT {
    PrecStackElem *top;
} PrecStack;

void initStack(PrecStack *stack);

void push(PrecStack *stack, PrecStackType type);

void pushAfterTerminal(PrecStack *stack, PrecStackType type);

void pop(PrecStack *stack);

PrecStackElem *topTerminal(PrecStack *stack);

void deleteStack(PrecStack *stack);

#endif // PRECSTACK_H
