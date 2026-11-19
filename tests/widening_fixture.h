/**
 * @file    widening_fixture.h
 * @brief   Hand-built AST for Int-to-Double conversion at value boundaries
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this test interface. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 */

#ifndef WIDENING_FIXTURE_H
#define WIDENING_FIXTURE_H

#include "../ast.h"

/// @brief Builds the widening program; free it with deleteNode().
ASTNode *wideningFixtureBuild(void);

#endif // WIDENING_FIXTURE_H
