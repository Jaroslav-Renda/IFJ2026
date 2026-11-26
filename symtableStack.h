/**
 * @file    symtableStack.h
 * @brief   Header file for stack of symtables
 * @author  Jakub Poláček (xpolacj00)
 * @details project_IFJ2026
 */
#ifndef SYMSTACK_H
#define SYMSTACK_H

#include "symtable.h"
#include <stdlib.h>

typedef struct SymtableStackT {
    Symtable *symtable;
    struct SymtableStackT *next;
} symtableStack;
typedef symtableStack *stackPtr;

// typedef so we can avoid using **stack
symtableStack *initSymtableStack();

/**
 * @brief pushes symtable to the stack initializes one item of stack
 * @param symtable pushed symtable
 * @param stackPtr adress of stackpointer (we need to change where it points)
 */
void push(Symtable *symtable, stackPtr *stack);

/**
 * @brief pops symtable from the stack and also frees its content
 * @param stackPtr adress of stackpointer (we need to change where it points)
 */
void pop(stackPtr *stack);

/**
 * @brief delets all of the stack and frees its content
 * @param stackPtr adress of stackpointer (we need to change where it points)
 */
void deleteStack(stackPtr *stack);

#endif // SYMSTACK_H
