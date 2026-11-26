/**
 * @file    symtableStack.c
 * @brief   Source file for implementation of Stack of symtables
 * @author  Jakub Poláček (xpolacj00)
 * @details project_IFJ2026
 */

#include "symtableStack.h"
#include "symtable.h"
#include <stdlib.h>
void push(Symtable *symtable, stackPtr *stack) {
    symtableStack *temp = (symtableStack *)malloc(sizeof(symtableStack));
    if (temp == NULL) {
        return;
    }
    temp->symtable = symtable;
    temp->next = *stack;
    *stack = temp;
}

void pop(stackPtr *stack) {
    if (*stack == NULL) {
        return;
    }
    stackPtr temp = *stack;
    *stack = (*stack)->next;
    symtableFree(temp->symtable);
    temp->next = NULL;
    free(temp);
}

void deleteStack(stackPtr *stack) {
    while (*stack != NULL) {
        pop(stack);
    }
}
