/**
 * @file    precedence.h
 * @brief   Header file precedence analysis (expression parsing)
 * @author  Jaroslav Renda (xrendaj00)
 * @details project_IFJ2026
 */

#ifndef PRECEDENCE_H
#define PRECEDENCE_H

#include "parser.h"

// NOTE: not yet implemented, ALWAYS returns true.
bool parseExpression(currentToken *cur, lookaheadToken *lat);

#endif // !PRECEDENCE_H
