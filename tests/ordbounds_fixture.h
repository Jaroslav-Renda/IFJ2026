/**
 * @file    ordbounds_fixture.h
 * @brief   Hand-built AST for ord on empty and out-of-range strings
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this test interface. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 */

#ifndef ORDBOUNDS_FIXTURE_H
#define ORDBOUNDS_FIXTURE_H

#include "../ast.h"

/// @brief Builds the ord-boundary program; free it with deleteNode().
ASTNode *ordboundsFixtureBuild(void);

#endif // ORDBOUNDS_FIXTURE_H
