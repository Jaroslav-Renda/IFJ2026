/**
 * @file    numeric_fixture.c
 * @brief   Hand-built AST for numeric conversion and comparison coverage
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this entire fixture. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 * Expected behavior was derived from IFJ26 sections 3 and 4.
 */

#include "numeric_fixture.h"
#include "fixture_common.h"

/// @brief Appends `print(label); print(value); print("\n")` to a statement chain.
static void appendPrint(ASTNode **stmts, const char *label, ASTNode *value) {
    fChain(stmts, fCall("print", fLitStr(label)));
    fChain(stmts, fCall("print", value));
    fChain(stmts, fCall("print", fLitStr("\n")));
}

/// @brief Builds an if/else that prints @p text only when the relation holds.
static ASTNode *buildRelationCheck(BinaryOpType operation, ASTNode *left, ASTNode *right,
                                   const char *text) {
    return fIf(fBinOp(operation, left, right), fBlock(fCall("print", fLitStr(text))),
               fBlock(fCall("print", fLitStr("wrong\n"))));
}

/**
 * @brief Builds `@main def numericExample(): Unit`.
 *
 * Covers Int and Double division in both mixed-operand directions, unary
 * minus on Double, truncation toward zero, toStr on all basic printable
 * types, raw IFJcode26 float output, and every relational operator with an
 * implicit Int-to-Double conversion.
 */
static ASTNode *buildNumericMain(void) {
    ASTNode *stmts = NULL;

    fChain(&stmts, fVarDecl("intDiv", TYPE_UNKNOWN, false, fBinOp(OP_DIV, fLitInt(7), fLitInt(2))));
    fChain(&stmts,
           fVarDecl("leftMix", TYPE_UNKNOWN, false, fBinOp(OP_DIV, fLitInt(7), fLitDouble(2.0))));
    fChain(&stmts,
           fVarDecl("rightMix", TYPE_UNKNOWN, false, fBinOp(OP_DIV, fLitDouble(7.0), fLitInt(2))));
    fChain(&stmts,
           fVarDecl("negative", TYPE_UNKNOWN, false, fUnaryOp(UOP_MINUS, fLitDouble(2.75))));
    fChain(&stmts,
           fVarDecl("truncatedPositive", TYPE_UNKNOWN, false, fCall("toInt", fLitDouble(3.99))));
    fChain(&stmts, fVarDecl("truncatedNegative", TYPE_UNKNOWN, false,
                            fCall("toInt", fUnaryOp(UOP_MINUS, fLitDouble(3.99)))));
    fChain(&stmts, fVarDecl("intText", TYPE_UNKNOWN, false, fCall("toStr", fLitInt(42))));
    fChain(&stmts, fVarDecl("stringText", TYPE_UNKNOWN, false, fCall("toStr", fLitStr("same"))));

    appendPrint(&stmts, "intDiv=", fVarRef("intDiv"));
    appendPrint(&stmts, "leftMix=", fCall("toStr", fVarRef("leftMix")));
    appendPrint(&stmts, "rightMix=", fCall("toStr", fVarRef("rightMix")));
    appendPrint(&stmts, "negative=", fCall("toStr", fVarRef("negative")));
    appendPrint(&stmts, "truncatedPositive=", fVarRef("truncatedPositive"));
    appendPrint(&stmts, "truncatedNegative=", fVarRef("truncatedNegative"));
    appendPrint(&stmts, "intText=", fVarRef("intText"));
    appendPrint(&stmts, "stringText=", fVarRef("stringText"));
    appendPrint(&stmts, "raw=", fVarRef("leftMix"));

    fChain(&stmts, buildRelationCheck(OP_LT, fLitInt(2), fLitDouble(2.5), "lt\n"));
    fChain(&stmts, buildRelationCheck(OP_GT, fLitDouble(2.5), fLitInt(2), "gt\n"));
    fChain(&stmts, buildRelationCheck(OP_LTE, fLitInt(2), fLitDouble(2.0), "le\n"));
    fChain(&stmts, buildRelationCheck(OP_GTE, fLitDouble(2.0), fLitInt(2), "ge\n"));
    fChain(&stmts, buildRelationCheck(OP_EQ, fLitInt(2), fLitDouble(2.0), "eq\n"));
    fChain(&stmts, buildRelationCheck(OP_NEQ, fLitDouble(2.0), fLitInt(3), "ne\n"));

    return fFuncDef("numericExample", TYPE_UNIT, NULL, fBlock(stmts), true);
}

ASTNode *numericFixtureBuild(void) {
    ASTNode *entry = buildNumericMain();
    if (entry == NULL) {
        return NULL;
    }

    ASTNode *root = createTree();
    if (root == NULL) {
        deleteNode(entry);
        return NULL;
    }
    root->data.program.functions = entry;
    root->data.program.funcCount = 1;
    return root;
}
