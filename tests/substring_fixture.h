/**
 * @file    substring_fixture.h
 * @brief   Hand-built AST for subStr start/end index semantics
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this test interface. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 */

#ifndef SUBSTRING_FIXTURE_H
#define SUBSTRING_FIXTURE_H

#include "../ast.h"

/// @brief Builds the substring program; free it with deleteNode().
ASTNode *substringFixtureBuild(void);

#endif // SUBSTRING_FIXTURE_H
