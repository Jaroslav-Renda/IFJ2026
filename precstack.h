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

/**
 * @enum PrecStackType
 * @brief Expression tokens types
 */
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

/**
 * @brief Representation of the precedence stack's element
 */
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

/**
 * @brief The stack for precedence analysis
 */
typedef struct PrecStackT {
    PrecStackElem *top;
} PrecStack;

/**
 * @brief Initializes the stack
 * @param stack Pointer to the stack to be initialiezed
 * @return True on success false otherwise
 */
bool precInitStack(PrecStack *stack);

/**
 * @brief Push element with the specified type  onto the stack
 * @param stack Pointer to the stack
 * @param type The type of the pushed element
 * @return True on success false otherwise
 */
bool precPush(PrecStack *stack, PrecStackType type);

/**
 * @breif Push element after the highest terminal (skip all Es)
 * @param stack Pointer to the stack
 * @param type The type of the pushed element
 * @return True on success false otherwise
 */
bool precPushAfterTerminal(PrecStack *stack, PrecStackType type);

/**
 * @brief Pops the top of the stack
 * @param stack Pointer to the stack
 * @return True on success false otherwise
 */
bool precPop(PrecStack *stack);

/**
 * @brief Searches for the top terminal and returns it
 * @param stack Pointer to the stuck
 * @return A PrecStackElem, the highest terminal
 */
PrecStackElem *topTerminal(PrecStack *stack);

/**
 * @brief Deletes the stack
 * @param stack Pointer to the stack to be deleted
 * @return True on success false otherwise
 */
bool precDeleteStack(PrecStack *stack);

#endif // PRECSTACK_H
