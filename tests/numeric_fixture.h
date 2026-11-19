/**
 * @file    numeric_fixture.h
 * @brief   Hand-built AST for numeric conversion and comparison coverage
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this test interface. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 */

#ifndef NUMERIC_FIXTURE_H
#define NUMERIC_FIXTURE_H

#include "../ast.h"

/// @brief Builds the numeric program; free the result with deleteNode().
ASTNode *numericFixtureBuild(void);

#endif // NUMERIC_FIXTURE_H
