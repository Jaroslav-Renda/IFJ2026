/**
 * @file    symtableStack.c
 * @brief   Header file for semanthical analysis
 * @author  Jakub Poláček (xpolacj00)
 * @details project_IFJ2026
 */

#ifndef SYMANALYS_H
#define SYMANALYS_H
#include "ast.h"
#include "symtable.h"
#include "symtableStack.h"
#include <stdint.h>

symtableItem *findVarInScopes(stackPtr stack, const char *identifier);
uint8_t semantAnalyse(ASTNode *program);
#endif // SYMANALYS_H
