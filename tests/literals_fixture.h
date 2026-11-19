/**
 * @file    literals_fixture.h
 * @brief   Hand-built AST for exact IFJcode26 string-literal escaping
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this test interface. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 */

#ifndef LITERALS_FIXTURE_H
#define LITERALS_FIXTURE_H

#include "../ast.h"

/// @brief Builds the literal program; free the result with deleteNode().
ASTNode *literalsFixtureBuild(void);

#endif // LITERALS_FIXTURE_H
