/**
 * @file    factorial_fixture.c
 * @brief   Hand-built AST of the official example basic/factorial.scala
 * @author  Vojtech Urich (xurich00)
 * @date    2026-11-18
 * @details project: IFJ2026
 *
 * Source of truth: IFJ26_examples/basic/factorial.scala
 * Reference IFJcode26 (NOT a golden file, our output may differ):
 * IFJ26_examples/basic/factorial.ifjcode
 * Expected program behavior: factorial.in -> factorial.out
 */

#include "factorial_fixture.h"
#include "fixture_common.h"

/* ------------------------------------------------- def factorialIterative */

/**
 * @brief Builds `def factorialIterative(n: Int): Int`.
 *
 * var result = 1
 * var i: Int = 2
 * while i <= n do
 *   result = result * i
 *   i = i + 1
 * result
 */
static ASTNode *buildFactorialIterative(void) {
    ASTNode *cond = fBinOp(OP_LTE, fVarRef("i"), fVarRef("n"));
    ASTNode *mul = fBinOp(OP_MUL, fVarRef("result"), fVarRef("i"));
    ASTNode *inc = fBinOp(OP_ADD, fVarRef("i"), fLitInt(1));

    ASTNode *bodyStmts = NULL;
    fChain(&bodyStmts, fAssign("result", mul));
    fChain(&bodyStmts, fAssign("i", inc));

    ASTNode *loop = fWhile(cond, fBlock(bodyStmts));
    ASTNode *declResult = fVarDecl("result", TYPE_UNKNOWN, true, fLitInt(1));
    ASTNode *declI = fVarDecl("i", TYPE_INT, true, fLitInt(2));

    ASTNode *stmts = NULL;
    fChain(&stmts, declResult);
    fChain(&stmts, declI);
    fChain(&stmts, loop);
    /* Implicit tail expression `result`: kept as a plain expression statement,
     * checkFunctionTail() reads its value from the last statement. */
    fChain(&stmts, fVarRef("result"));

    ASTNode *params = fParam("n", TYPE_INT);
    return fFuncDef("factorialIterative", TYPE_INT, params, fBlock(stmts), false);
}

/* ------------------------------------------------- def factorialRecursive */

/**
 * @brief Builds `def factorialRecursive(n: Int): Int`.
 *
 * if n <= 1 then
 *   return 1
 * else
 *   val previous = n - 1
 *   val smaller = factorialRecursive(previous)  // call as the whole init expr
 *   return n * smaller
 */
static ASTNode *buildFactorialRecursive(void) {
    ASTNode *thenBody = fBlock(fReturn(fLitInt(1)));

    ASTNode *declPrevious =
        fVarDecl("previous", TYPE_UNKNOWN, false, fBinOp(OP_SUB, fVarRef("n"), fLitInt(1)));
    ASTNode *declSmaller =
        fVarDecl("smaller", TYPE_UNKNOWN, false, fCall("factorialRecursive", fVarRef("previous")));
    ASTNode *ret = fReturn(fBinOp(OP_MUL, fVarRef("n"), fVarRef("smaller")));

    ASTNode *elseStmts = NULL;
    fChain(&elseStmts, declPrevious);
    fChain(&elseStmts, declSmaller);
    fChain(&elseStmts, ret);

    ASTNode *cond = fBinOp(OP_LTE, fVarRef("n"), fLitInt(1));
    ASTNode *condIf = fIf(cond, thenBody, fBlock(elseStmts));

    ASTNode *params = fParam("n", TYPE_INT);
    return fFuncDef("factorialRecursive", TYPE_INT, params, fBlock(condIf), false);
}

/* --------------------------------------------- @main def factorialExample */

/**
 * @brief Builds `@main def factorialExample(): Unit`.
 *
 * The @main body ends with a print call, so it has no explicit return;
 * codegen closes it with EXIT int@0.
 */
static ASTNode *buildMain(void) {
    static const char *const usage = "Use an integer from 0 to 12.\n";

    ASTNode *declN = fVarDecl("n", TYPE_UNKNOWN, false, fCall("readInt", NULL));

    /* if n > 12 -> usage message */
    ASTNode *innerThen = fBlock(fCall("print", fLitStr(usage)));

    ASTNode *outerElseStmts = NULL;
    fChain(&outerElseStmts,
           fVarDecl("iterative", TYPE_UNKNOWN, false, fCall("factorialIterative", fVarRef("n"))));
    fChain(&outerElseStmts,
           fVarDecl("recursive", TYPE_UNKNOWN, false, fCall("factorialRecursive", fVarRef("n"))));
    fChain(&outerElseStmts, fCall("print", fLitStr("iterative: ")));
    fChain(&outerElseStmts, fCall("print", fVarRef("iterative")));
    fChain(&outerElseStmts, fCall("print", fLitStr("\nrecursive: ")));
    fChain(&outerElseStmts, fCall("print", fVarRef("recursive")));
    fChain(&outerElseStmts, fCall("print", fLitStr("\n")));

    ASTNode *innerIf =
        fIf(fBinOp(OP_GT, fVarRef("n"), fLitInt(12)), innerThen, fBlock(outerElseStmts));

    ASTNode *outerIf = fIf(fBinOp(OP_LT, fVarRef("n"), fLitInt(0)),
                           fBlock(fCall("print", fLitStr(usage))), fBlock(innerIf));

    ASTNode *stmts = NULL;
    fChain(&stmts, declN);
    fChain(&stmts, outerIf);

    return fFuncDef("factorialExample", TYPE_UNIT, NULL, fBlock(stmts), true);
}

/* ------------------------------------------------------------------- root */

ASTNode *factorialFixtureBuild(void) {
    ASTNode *root = createTree();
    if (root == NULL) {
        return NULL;
    }

    /* On allocation failure everything already attached to the root is freed;
     * a subtree that is built but not attached yet is leaked, which is
     * acceptable here because the caller aborts immediately. */
    ASTNode *iter = buildFactorialIterative();
    ASTNode *recur = buildFactorialRecursive();
    ASTNode *entry = buildMain();
    if (iter == NULL || recur == NULL || entry == NULL) {
        deleteNode(iter);
        deleteNode(recur);
        deleteNode(entry);
        deleteNode(root);
        return NULL;
    }

    ASTNode *functions = NULL;
    fChain(&functions, iter);
    fChain(&functions, recur);
    fChain(&functions, entry);

    root->data.program.functions = functions;
    root->data.program.funcCount = 3;
    return root;
}
