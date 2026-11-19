/**
 * @file    strings_fixture.h
 * @brief   Hand-built AST of the official example basic/strings.scala
 * @author  Vojtech Urich (xurich00)
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * Source of truth: IFJ26_examples/basic/strings.scala
 * Reference IFJcode26 (NOT a golden file, our output may differ):
 * IFJ26_examples/basic/strings.ifjcode
 * Expected program behavior: tests/strings.in -> tests/strings.out
 */

#ifndef STRINGS_FIXTURE_H
#define STRINGS_FIXTURE_H

#include "../ast.h"

/**
 * @brief Builds the AST of basic/strings.scala in its post-parse state.
 *
 * Exercises: string builtins (strLength, ord, chr, substr, toStr, readLine),
 * string concatenation, the implicit Int->Double conversion (`length / 2.0`),
 * nested if/else with early returns, a while loop, an empty `()` else block,
 * Unit functions without return, and an implicit tail expression.
 *
 * The source spells the builtin `subStr`, the semantAnalys.c builtin table
 * registers it as `substr`; the fixture uses the registered spelling.
 *
 * @return Newly allocated NODE_PROGRAM root, or NULL on allocation failure.
 *         Free the whole tree with deleteNode().
 */
ASTNode *stringsFixtureBuild(void);

#endif // STRINGS_FIXTURE_H
