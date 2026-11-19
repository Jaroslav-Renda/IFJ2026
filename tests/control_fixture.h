/**
 * @file    control_fixture.h
 * @brief   Hand-built AST of a synthetic control-flow-focused program
 * @author  Vojtech Urich (xurich00)
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * Synthetic fixture (no official counterpart): exercises nested if/else,
 * a while loop with a compound condition, an early return from inside a
 * loop body, a return after the loop as the explicit function tail, and
 * comparisons mixed with arithmetic (the "even" test via i / 2 * 2 == i).
 */

#ifndef CONTROL_FIXTURE_H
#define CONTROL_FIXTURE_H

#include "../ast.h"

/**
 * @brief Builds the synthetic control-flow program in its post-parse state.
 *
 * @return Newly allocated NODE_PROGRAM root, or NULL on allocation failure.
 *         Free the whole tree with deleteNode().
 */
ASTNode *controlFixtureBuild(void);

#endif // CONTROL_FIXTURE_H
