/**
 * @file    symtableStack.c
 * @brief   Header file for semanthical analysis
 * @author  Jakub Poláček (xpolacj00)
 * @date    2026-11-6
 * @details project: IFJ2026
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
