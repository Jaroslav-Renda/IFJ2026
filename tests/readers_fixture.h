/**
 * @file    readers_fixture.h
 * @brief   Hand-built AST for valid, malformed, and EOF input reads
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this test interface. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 */

#ifndef READERS_FIXTURE_H
#define READERS_FIXTURE_H

#include "../ast.h"

/// @brief Builds the input program; free the result with deleteNode().
ASTNode *readersFixtureBuild(void);

#endif // READERS_FIXTURE_H
