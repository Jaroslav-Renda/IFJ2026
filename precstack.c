/**
 * @file    precstack.c
 * @brief   Implementation of precedence analysis stack
 * @author  Jaroslav Renda (xrendaj00)
 * @details project: IFJ2026
 */

#include "precstack.h"
#include "error.h"
#include <stdlib.h>

bool precInitStack(PrecStack *stack) {
    if (stack == NULL) {
        errorInternal(0, "stack pointer is NULL");
        return false;
    }

    stack->top = NULL;
    return true;
}

bool precPush(PrecStack *stack, PrecStackType type) {
    if (stack == NULL) {
        errorInternal(0, "stack pointer is NULL");
        return false;
    }

    PrecStackElem *newEl = (PrecStackElem *)malloc(sizeof(PrecStackElem));
    if (newEl == NULL) {
        errorInternal(0, "memory allocation failed");
        return false;
    }

    newEl->type = type;
    newEl->isFooCall = false;
    newEl->next = stack->top;
    stack->top = newEl;
    return true;
}

bool precPushAfterTerminal(PrecStack *stack, PrecStackType type) {
    if (stack == NULL) {
        errorInternal(0, "stack pointer is NULL");
        return false;
    }

    PrecStackElem **cur = &stack->top;

    while (*cur != NULL && ((*cur)->type == P_STACK_EXP)) {
        cur = &(*cur)->next;
    }

    PrecStackElem *newEl = (PrecStackElem *)malloc(sizeof(PrecStackElem));
    if (newEl == NULL) {
        errorInternal(0, "memory allocation failed");
        return false;
    }

    newEl->type = type;
    newEl->isFooCall = false;
    newEl->next = *cur;
    *cur = newEl;
    return true;
}

bool precPop(PrecStack *stack) {
    if (stack == NULL) {
        errorInternal(0, "stack pointer is NULL");
        return false;
    }

    if (stack->top == NULL) {
        errorInternal(0, "cannot precPop from an empty stack");
        return false;
    }

    PrecStackElem *tmp = stack->top;
    stack->top = stack->top->next;
    free(tmp);
    return true;
}

PrecStackElem *topTerminal(PrecStack *stack) {
    if (stack == NULL) {
        return NULL;
    }

    if (stack->top == NULL) {
        return NULL;
    }
    PrecStackElem *cur = stack->top;
    while (cur != NULL && (cur->type == P_STACK_EXP)) {
        cur = cur->next;
    }
    return cur;
}

bool precDeleteStack(PrecStack *stack) {
    if (stack == NULL) {
        errorInternal(0, "stack pointer is NULL");
        return false;
    }

    while (stack->top != NULL) {
        precPop(stack);
    }
    return true;
}
