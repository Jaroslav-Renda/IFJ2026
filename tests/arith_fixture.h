/**
 * @file    arith_fixture.h
 * @brief   Hand-built AST of a synthetic arithmetic-focused program
 * @author  Vojtech Urich (xurich00)
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * Synthetic fixture (no official counterpart): exercises every binary
 * operator on Int, Int/Double mixing with the implicit conversion, Int vs
 * Double division, unary minus/plus, operator nesting (precedence is baked
 * into the tree shape) and passing Int results through a user function.
 */

#ifndef ARITH_FIXTURE_H
#define ARITH_FIXTURE_H

#include "../ast.h"

/**
 * @brief Builds the synthetic arithmetic program in its post-parse state.
 *
 * @return Newly allocated NODE_PROGRAM root, or NULL on allocation failure.
 *         Free the whole tree with deleteNode().
 */
ASTNode *arithFixtureBuild(void);

#endif // ARITH_FIXTURE_H
