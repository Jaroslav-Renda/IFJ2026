/**
 * @file    looplocals_fixture.h
 * @brief   Hand-built AST for declarations executed in repeated loop bodies
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this test interface. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 */

#ifndef LOOPLOCALS_FIXTURE_H
#define LOOPLOCALS_FIXTURE_H

#include "../ast.h"

/// @brief Builds the loop-local program; free it with deleteNode().
ASTNode *looplocalsFixtureBuild(void);

#endif // LOOPLOCALS_FIXTURE_H
