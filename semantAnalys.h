/**
 * @file    symtableStack.c
 * @brief   Header file for semanthical analysis
 * @author  Jakub Poláček (xpolacj00)
 * @date    2026-11-6
 * @details project: IFJ2026
 */

#include "ast.h"
#include "symtable.h"
#include "symtableStack.h"
#ifndef SYMANALYS_H
#define SYMANALYS_H

symtableItem *findVarInScopes(stackPtr stack, const char *identifier);
int semantAnalyse(ASTNode *program);
#endif // SYMANALYS_H
