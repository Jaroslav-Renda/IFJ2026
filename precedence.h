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

/**
 * @brief Parses an expression
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @param lat Pointer to the lookahead token structure.
 * @param[in,out] expr Abstract syntax tree node to be created
 * @return true if the expression parsed successfuly.
 */
bool parseExpression(currentToken *cur, lookaheadToken *lat, ASTNode **expr);

/**
 * @brief Resolve incoming token based on current stack.
 *
 * @param token Pointer to the current token.
 * @param stack Pointer to the precedence stack.
 * @return true the token could be resolved.
 */
bool resolveToken(Token *token, PrecStack *stack);

/**
 * @brief Keeps reducing the stack until it is empty.
 *
 * @param stack Pointer to the precedence stack.
 * @param[in,out] expr Abstract syntax tree node to be updated
 * @return true if the stack could be reduced.
 */
bool finishExpression(PrecStack *stack, ASTNode **expr);

#endif // !PRECEDENCE_H
