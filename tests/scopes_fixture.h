/**
 * @file    scopes_fixture.h
 * @brief   Hand-built AST for shadowing in sibling and nested blocks
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this test interface. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 */

#ifndef SCOPES_FIXTURE_H
#define SCOPES_FIXTURE_H

#include "../ast.h"

/// @brief Builds the scope program; free the result with deleteNode().
ASTNode *scopesFixtureBuild(void);

#endif // SCOPES_FIXTURE_H
