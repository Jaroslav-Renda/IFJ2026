/**
 * @file    factorial_fixture.h
 * @brief   Hand-built AST of the official example basic/factorial.scala
 * @author  Vojtech Urich (xurich00)
 * @date    2026-11-18
 * @details project: IFJ2026
 *
 * Lets the code generator be tested before the lexer and the recursive-descent
 * parser are finished: the tree is built directly, then the real semantic
 * analysis fills in every semantic field, then codegen runs on the result.
 */

#ifndef FACTORIAL_FIXTURE_H
#define FACTORIAL_FIXTURE_H

#include "../ast.h"

/**
 * @brief Builds the AST of basic/factorial.scala (IFJ26_examples).
 *
 * The tree is in the state it has right after parsing: only the fields the
 * parser is responsible for are filled (names, node types, literal values,
 * declared types, isMain). semanticType and convertToDouble are left zero,
 * exactly as the parser leaves them, so the fixture has to be passed through
 * semantAnalyse() before it can be handed to codegenGenerate().
 *
 * Two representation decisions are baked in and the parser has to match them:
 *  - every function body, then/else branch and loop body is wrapped in
 *    NODE_BLOCK (semantAnalys.c:unwrapBlock() also accepts a bare statement
 *    chain, so both forms stay analysable);
 *  - the implicit tail expression of an expression-bodied function stays a
 *    plain expression statement (see checkFunctionTail() rule "default"), it
 *    is NOT turned into a NODE_RETURN by the parser.
 *
 * @return Newly allocated NODE_PROGRAM root, or NULL on allocation failure.
 *         Free the whole tree with deleteNode().
 */
ASTNode *factorialFixtureBuild(void);

#endif // FACTORIAL_FIXTURE_H
