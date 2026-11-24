/**
 * @file    precedence.h
 * @brief   Header file precedence analysis (expression parsing)
 * @author  Jaroslav Renda (xrendaj00)
 * @details project_IFJ2026
 */

#ifndef PRECEDENCE_H
#define PRECEDENCE_H

#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "precstack.h"

// NOTE: not yet implemented, ALWAYS returns true.
bool parseExpression(currentToken *cur, lookaheadToken *lat, ASTNode **expr);

bool resolveToken(Token *token, PrecStack *stack);

bool finishExpression(PrecStack *stack, ASTNode **expr);

#endif // !PRECEDENCE_H
