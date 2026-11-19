/**
 * @file    functions_fixture.c
 * @brief   Hand-built AST for user-function calling conventions
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this entire fixture. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 */

#include "functions_fixture.h"
#include "fixture_common.h"

/// @brief Builds `def pack(a: Int, b: Int, c: Int): Int = a*100 + b*10 + c`.
static ASTNode *buildPack(void) {
    ASTNode *params = fParam("a", TYPE_INT);
    fChain(&params, fParam("b", TYPE_INT));
    fChain(&params, fParam("c", TYPE_INT));

    ASTNode *left = fBinOp(OP_MUL, fVarRef("a"), fLitInt(100));
    ASTNode *middle = fBinOp(OP_MUL, fVarRef("b"), fLitInt(10));
    ASTNode *sum = fBinOp(OP_ADD, fBinOp(OP_ADD, left, middle), fVarRef("c"));
    return fFuncDef("pack", TYPE_INT, params, fBlock(sum), false);
}

/// @brief Builds a zero-argument function with an implicit literal result.
static ASTNode *buildAnswer(void) {
    return fFuncDef("answer", TYPE_INT, NULL, fBlock(fLitInt(42)), false);
}

/// @brief Builds a three-parameter function returning `number|text|ratio`.
static ASTNode *buildDescribe(void) {
    ASTNode *params = fParam("number", TYPE_INT);
    fChain(&params, fParam("text", TYPE_STRING));
    fChain(&params, fParam("ratio", TYPE_DOUBLE));

    ASTNode *result = fBinOp(OP_ADD, fCall("toStr", fVarRef("number")), fLitStr("|"));
    result = fBinOp(OP_ADD, result, fVarRef("text"));
    result = fBinOp(OP_ADD, result, fLitStr("|"));
    result = fBinOp(OP_ADD, result, fCall("toStr", fVarRef("ratio")));
    return fFuncDef("describe", TYPE_STRING, params, fBlock(result), false);
}

/// @brief Builds a Unit function whose bare return skips an unreachable print.
static ASTNode *buildStopAfterPrint(void) {
    ASTNode *stmts = NULL;
    fChain(&stmts, fCall("print", fVarRef("text")));
    fChain(&stmts, fReturn(NULL));
    fChain(&stmts, fCall("print", fLitStr("unreachable\n")));
    return fFuncDef("stopAfterPrint", TYPE_UNIT, fParam("text", TYPE_STRING), fBlock(stmts), false);
}

/// @brief Appends one expression and a newline to a statement chain.
static void appendPrint(ASTNode **stmts, ASTNode *value) {
    fChain(stmts, fCall("print", value));
    fChain(stmts, fCall("print", fLitStr("\n")));
}

static ASTNode *buildFunctionsMain(void) {
    ASTNode *stmts = NULL;
    fChain(&stmts, fVarDecl("packedLiteral", TYPE_UNKNOWN, false,
                            fCall("pack", fArgs3(fLitInt(1), fLitInt(2), fLitInt(3)))));
    fChain(&stmts, fVarDecl("packedVariables", TYPE_UNKNOWN, false,
                            fCall("pack", fArgs3(fLitInt(4), fLitInt(5), fLitInt(6)))));
    fChain(&stmts, fVarDecl("zeroArg", TYPE_UNKNOWN, false, fCall("answer", NULL)));
    fChain(&stmts,
           fVarDecl("description", TYPE_UNKNOWN, false,
                    fCall("describe", fArgs3(fLitInt(7), fLitStr("eighty"), fLitDouble(2.5)))));

    appendPrint(&stmts, fVarRef("packedLiteral"));
    appendPrint(&stmts, fVarRef("packedVariables"));
    appendPrint(&stmts, fVarRef("zeroArg"));
    appendPrint(&stmts, fVarRef("description"));
    fChain(&stmts, fCall("stopAfterPrint", fLitStr("before-stop\n")));
    fChain(&stmts, fCall("print", fLitStr("after-stop\n")));

    return fFuncDef("functionsExample", TYPE_UNIT, NULL, fBlock(stmts), true);
}

ASTNode *functionsFixtureBuild(void) {
    ASTNode *functions = NULL;
    fChain(&functions, buildPack());
    fChain(&functions, buildAnswer());
    fChain(&functions, buildDescribe());
    fChain(&functions, buildStopAfterPrint());
    fChain(&functions, buildFunctionsMain());

    ASTNode *root = createTree();
    if (root == NULL) {
        deleteNode(functions);
        return NULL;
    }
    root->data.program.functions = functions;
    root->data.program.funcCount = 5;
    return root;
}
