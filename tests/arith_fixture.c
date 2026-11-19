/**
 * @file    arith_fixture.c
 * @brief   Hand-built AST of a synthetic arithmetic-focused program
 * @author  Vojtech Urich (xurich00)
 * @date    2026-11-19
 * @details project: IFJ2026
 */

#include "arith_fixture.h"
#include "fixture_common.h"

/* ------------------------------------------------------------ def twice */

/**
 * @brief Builds `def twice(n: Int): Int`.
 *
 * n * 2
 */
static ASTNode *buildTwice(void) {
    ASTNode *body = fBlock(fBinOp(OP_MUL, fVarRef("n"), fLitInt(2)));
    return fFuncDef("twice", TYPE_INT, fParam("n", TYPE_INT), body, false);
}

/* ------------------------------------------------- @main def arithExample */

/**
 * @brief Builds `@main def arithExample(): Unit`.
 *
 * val a = 6 + 7 * 3        // 27  (precedence baked into the tree)
 * val b = (6 + 7) * 3      // 39  (parentheses are a parser detail)
 * val c = 10 / 4           // 2   (Int division truncates)
 * val d = 10 / 4.0         // 2.5 (implicit Int->Double on the left operand)
 * val e = 0 - 5 + 2        // -3  (left associativity)
 * val f = 2.5 * 2          // 5.0 (Int operand converted)
 * val g = twice(a) - b     // 15
 * val h = -a               // -27 (unary minus on a variable)
 * val i = +b               // 39  (unary plus is the identity)
 * print(a); print("\n"); print(b); print("\n"); print(c); print("\n");
 * print(g); print("\n"); print(h); print("\n"); print(i); print("\n")
 *
 * Only Int results are printed: the textual format of a Double print is not
 * pinned down, so d and f are computed (codegen coverage) but not printed.
 */
static ASTNode *buildArithMain(void) {
    ASTNode *declA = fVarDecl("a", TYPE_UNKNOWN, false,
                              fBinOp(OP_ADD, fLitInt(6), fBinOp(OP_MUL, fLitInt(7), fLitInt(3))));
    ASTNode *declB = fVarDecl("b", TYPE_UNKNOWN, false,
                              fBinOp(OP_MUL, fBinOp(OP_ADD, fLitInt(6), fLitInt(7)), fLitInt(3)));
    ASTNode *declC = fVarDecl("c", TYPE_UNKNOWN, false, fBinOp(OP_DIV, fLitInt(10), fLitInt(4)));
    /* 10 / 4.0: semant marks the Int operand with convertToDouble. */
    ASTNode *declD =
        fVarDecl("d", TYPE_UNKNOWN, false, fBinOp(OP_DIV, fLitInt(10), fLitDouble(4.0)));
    ASTNode *declE = fVarDecl("e", TYPE_UNKNOWN, false,
                              fBinOp(OP_ADD, fBinOp(OP_SUB, fLitInt(0), fLitInt(5)), fLitInt(2)));
    ASTNode *declF =
        fVarDecl("f", TYPE_UNKNOWN, false, fBinOp(OP_MUL, fLitDouble(2.5), fLitInt(2)));
    ASTNode *declG = fVarDecl("g", TYPE_UNKNOWN, false,
                              fBinOp(OP_SUB, fCall("twice", fVarRef("a")), fVarRef("b")));
    ASTNode *declH = fVarDecl("h", TYPE_UNKNOWN, false, fUnaryOp(UOP_MINUS, fVarRef("a")));
    ASTNode *declI = fVarDecl("i", TYPE_UNKNOWN, false, fUnaryOp(UOP_PLUS, fVarRef("b")));

    ASTNode *stmts = NULL;
    fChain(&stmts, declA);
    fChain(&stmts, declB);
    fChain(&stmts, declC);
    fChain(&stmts, declD);
    fChain(&stmts, declE);
    fChain(&stmts, declF);
    fChain(&stmts, declG);
    fChain(&stmts, declH);
    fChain(&stmts, declI);

    static const char *const printed[] = {"a", "b", "c", "g", "h", "i"};
    for (size_t index = 0; index < sizeof(printed) / sizeof(printed[0]); index++) {
        fChain(&stmts, fCall("print", fVarRef(printed[index])));
        fChain(&stmts, fCall("print", fLitStr("\n")));
    }

    return fFuncDef("arithExample", TYPE_UNIT, NULL, fBlock(stmts), true);
}

/* ------------------------------------------------------------------- root */

ASTNode *arithFixtureBuild(void) {
    ASTNode *root = createTree();
    if (root == NULL) {
        return NULL;
    }

    ASTNode *twice = buildTwice();
    ASTNode *entry = buildArithMain();
    if (twice == NULL || entry == NULL) {
        deleteNode(twice);
        deleteNode(entry);
        deleteNode(root);
        return NULL;
    }

    ASTNode *functions = NULL;
    fChain(&functions, twice);
    fChain(&functions, entry);

    root->data.program.functions = functions;
    root->data.program.funcCount = 2;
    return root;
}
