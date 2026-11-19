/**
 * @file    precstack.c
 * @brief   Implementation of precedence analysis stack
 * @author  Jaroslav Renda (xrendaj00)
 * @details project: IFJ2026
 */

#include "precstack.h"
#include <stdlib.h>

void initStack(PrecStack *stack) {
    if (stack == NULL) {
        return;
    }

    stack->top = NULL;
}

void push(PrecStack *stack, PrecStackType type) {
    if (stack == NULL) {
        return;
    }

    PrecStackElem *newEl = (PrecStackElem *)malloc(sizeof(PrecStackElem));
    if (newEl == NULL) {
        return;
    }

    newEl->type = type;
    newEl->isFooCall = false;
    newEl->next = stack->top;
    stack->top = newEl;
}

void pushAfterTerminal(PrecStack *stack, PrecStackType type) {
    if (stack == NULL) {
        return;
    }

    PrecStackElem **cur = &stack->top;

    while (*cur != NULL && ((*cur)->type == P_EXP)) {
        cur = &(*cur)->next;
    }

    PrecStackElem *newEl = (PrecStackElem *)malloc(sizeof(PrecStackElem));
    if (newEl == NULL) {
        return;
    }

    newEl->type = type;
    newEl->isFooCall = false;
    newEl->next = *cur;
    *cur = newEl;
}

void pop(PrecStack *stack) {
    if (stack == NULL) {
        return;
    }

    if (stack->top == NULL) {
        return;
    }

    PrecStackElem *tmp = stack->top;
    stack->top = stack->top->next;
    free(tmp);
}

PrecStackElem *topTerminal(PrecStack *stack) {
    if (stack == NULL) {
        return NULL;
    }

    if (stack->top == NULL) {
        return NULL;
    }
    PrecStackElem *cur = stack->top;
    while (cur != NULL && (cur->type == P_EXP)) {
        cur = cur->next;
    }
    return cur;
}

void deleteStack(PrecStack *stack) {
    if (stack == NULL) {
        return;
    }

    while (stack->top != NULL) {
        pop(stack);
    }
}
