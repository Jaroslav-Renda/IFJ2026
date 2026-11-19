/**
 * @file    control_fixture.c
 * @brief   Hand-built AST of a synthetic control-flow-focused program
 * @author  Vojtech Urich (xurich00)
 * @date    2026-11-19
 * @details project: IFJ2026
 */

#include "control_fixture.h"
#include "fixture_common.h"

/* -------------------------------------------------------- def classify */

/**
 * @brief Builds `def classify(n: Int): String`.
 *
 * if n < 0 then
 *   return "neg"
 * else
 *   if n == 0 then
 *     return "zero"
 *   else
 *     return "pos"
 *
 * A nested if/else where every branch returns; the outer if is the
 * implicit tail of the function.
 */
static ASTNode *buildClassify(void) {
    ASTNode *outerThen = fBlock(fReturn(fLitStr("neg")));

    ASTNode *innerThen = fBlock(fReturn(fLitStr("zero")));
    ASTNode *innerElse = fBlock(fReturn(fLitStr("pos")));
    ASTNode *innerIf = fIf(fBinOp(OP_EQ, fVarRef("n"), fLitInt(0)), innerThen, innerElse);

    ASTNode *outerIf = fIf(fBinOp(OP_LT, fVarRef("n"), fLitInt(0)), outerThen, fBlock(innerIf));

    return fFuncDef("classify", TYPE_STRING, fParam("n", TYPE_INT), fBlock(outerIf), false);
}

/* -------------------------------------------------- def sumUntilSquare */

/**
 * @brief Builds `def sumUntilSquare(limit: Int): Int`.
 *
 * var sum = 0
 * var i = 1
 * while i <= limit do
 *   if i * i > 20 then
 *     return sum     // early return from inside a loop body
 *   sum = sum + i * i
 *   i = i + 1
 * return sum         // explicit return as the function tail
 *
 * For limit >= 5 the early return fires with sum = 1 + 4 + 9 + 16 = 30.
 */
static ASTNode *buildSumUntilSquare(void) {
    ASTNode *declSum = fVarDecl("sum", TYPE_UNKNOWN, true, fLitInt(0));
    ASTNode *declI = fVarDecl("i", TYPE_UNKNOWN, true, fLitInt(1));

    ASTNode *earlyReturn = fBlock(fReturn(fVarRef("sum")));
    ASTNode *squareCheck = fIf(
        fBinOp(OP_GT, fBinOp(OP_MUL, fVarRef("i"), fVarRef("i")), fLitInt(20)), earlyReturn, NULL);

    ASTNode *body = NULL;
    fChain(&body, squareCheck);
    fChain(&body, fAssign("sum", fBinOp(OP_ADD, fVarRef("sum"),
                                        fBinOp(OP_MUL, fVarRef("i"), fVarRef("i")))));
    fChain(&body, fAssign("i", fBinOp(OP_ADD, fVarRef("i"), fLitInt(1))));

    ASTNode *loop = fWhile(fBinOp(OP_LTE, fVarRef("i"), fVarRef("limit")), fBlock(body));

    ASTNode *stmts = NULL;
    fChain(&stmts, declSum);
    fChain(&stmts, declI);
    fChain(&stmts, loop);
    fChain(&stmts, fReturn(fVarRef("sum")));

    return fFuncDef("sumUntilSquare", TYPE_INT, fParam("limit", TYPE_INT), fBlock(stmts), false);
}

/* ------------------------------------------------ @main def controlExample */

/**
 * @brief Builds `@main def controlExample(): Unit`.
 *
 * var i = 0
 * var sum = 0
 * while i < 10 do
 *   if i / 2 * 2 == i then  // "even" test through Int division
 *     sum = sum + i
 *   i = i + 1
 * print(sum)               // 0 + 2 + 4 + 6 + 8 = 20
 * print("\n")
 * val neg = 0 - 1          // the basic task forbids expressions as call args
 * print(classify(sum))     // pos
 * print("\n")
 * print(classify(neg))     // neg
 * print("\n")
 * print(classify(0))       // zero
 * print("\n")
 * print(sumUntilSquare(100))  // 30
 * print("\n")
 */
static ASTNode *buildControlMain(void) {
    ASTNode *declI = fVarDecl("i", TYPE_UNKNOWN, true, fLitInt(0));
    ASTNode *declSum = fVarDecl("sum", TYPE_UNKNOWN, true, fLitInt(0));

    ASTNode *evenIf =
        fIf(fBinOp(OP_EQ, fBinOp(OP_MUL, fBinOp(OP_DIV, fVarRef("i"), fLitInt(2)), fLitInt(2)),
                   fVarRef("i")),
            fBlock(fAssign("sum", fBinOp(OP_ADD, fVarRef("sum"), fVarRef("i")))), NULL);

    ASTNode *body = NULL;
    fChain(&body, evenIf);
    fChain(&body, fAssign("i", fBinOp(OP_ADD, fVarRef("i"), fLitInt(1))));

    ASTNode *loop = fWhile(fBinOp(OP_LT, fVarRef("i"), fLitInt(10)), fBlock(body));

    ASTNode *stmts = NULL;
    fChain(&stmts, declI);
    fChain(&stmts, declSum);
    fChain(&stmts, loop);
    fChain(&stmts, fCall("print", fVarRef("sum")));
    fChain(&stmts, fCall("print", fLitStr("\n")));
    fChain(&stmts, fVarDecl("neg", TYPE_UNKNOWN, false, fBinOp(OP_SUB, fLitInt(0), fLitInt(1))));
    fChain(&stmts, fCall("print", fCall("classify", fVarRef("sum"))));
    fChain(&stmts, fCall("print", fLitStr("\n")));
    fChain(&stmts, fCall("print", fCall("classify", fVarRef("neg"))));
    fChain(&stmts, fCall("print", fLitStr("\n")));
    fChain(&stmts, fCall("print", fCall("classify", fLitInt(0))));
    fChain(&stmts, fCall("print", fLitStr("\n")));
    fChain(&stmts, fCall("print", fCall("sumUntilSquare", fLitInt(100))));
    fChain(&stmts, fCall("print", fLitStr("\n")));

    return fFuncDef("controlExample", TYPE_UNIT, NULL, fBlock(stmts), true);
}

/* ------------------------------------------------------------------- root */

ASTNode *controlFixtureBuild(void) {
    ASTNode *root = createTree();
    if (root == NULL) {
        return NULL;
    }

    ASTNode *built[3];
    built[0] = buildClassify();
    built[1] = buildSumUntilSquare();
    built[2] = buildControlMain();

    ASTNode *functions = NULL;
    for (size_t index = 0; index < 3; index++) {
        if (built[index] == NULL) {
            for (size_t cleanup = 0; cleanup < 3; cleanup++) {
                deleteNode(built[cleanup]);
            }
            deleteNode(root);
            return NULL;
        }
        fChain(&functions, built[index]);
    }

    root->data.program.functions = functions;
    root->data.program.funcCount = 3;
    return root;
}
