/**
 * @file    strings_fixture.c
 * @brief   Hand-built AST of the official example basic/strings.scala
 * @author  Vojtech Urich (xurich00)
 * @date    2026-11-19
 * @details project: IFJ2026
 */

#include "strings_fixture.h"
#include "fixture_common.h"

/* ------------------------------------------------- def withoutLineFeed */

/**
 * @brief Builds `def withoutLineFeed(text: String): String`.
 *
 * val length = strLength(text)
 * if length == 0 then
 *   return text
 * else
 *   val last = length - 1
 *   val code = ord(text, last)
 *   if code == 10 then
 *     return substr(text, 0, last)
 *   else
 *     return text
 *
 * The function tail is an if statement whose every branch returns; semant
 * has to accept an if as the implicit tail expression of a String function.
 */
static ASTNode *buildWithoutLineFeed(void) {
    ASTNode *declLength =
        fVarDecl("length", TYPE_UNKNOWN, false, fCall("strLength", fVarRef("text")));
    ASTNode *thenBody = fBlock(fReturn(fVarRef("text")));

    ASTNode *declLast =
        fVarDecl("last", TYPE_UNKNOWN, false, fBinOp(OP_SUB, fVarRef("length"), fLitInt(1)));
    ASTNode *declCode = fVarDecl("code", TYPE_UNKNOWN, false,
                                 fCall("ord", fArgs2(fVarRef("text"), fVarRef("last"))));

    ASTNode *innerThen =
        fBlock(fReturn(fCall("substr", fArgs3(fVarRef("text"), fLitInt(0), fVarRef("last")))));
    ASTNode *innerElse = fBlock(fReturn(fVarRef("text")));
    ASTNode *innerIf = fIf(fBinOp(OP_EQ, fVarRef("code"), fLitInt(10)), innerThen, innerElse);

    ASTNode *elseStmts = NULL;
    fChain(&elseStmts, declLast);
    fChain(&elseStmts, declCode);
    fChain(&elseStmts, innerIf);

    ASTNode *outerIf =
        fIf(fBinOp(OP_EQ, fVarRef("length"), fLitInt(0)), thenBody, fBlock(elseStmts));

    ASTNode *stmts = NULL;
    fChain(&stmts, declLength);
    fChain(&stmts, outerIf);

    return fFuncDef("withoutLineFeed", TYPE_STRING, fParam("text", TYPE_STRING), fBlock(stmts),
                    false);
}

/* --------------------------------------------------------- def reverse */

/**
 * @brief Builds `def reverse(text: String): String`.
 *
 * val length = strLength(text)
 * var index = length - 1
 * var result = ""
 * while index >= 0 do
 *   val code = ord(text, index)
 *   val character = chr(code)
 *   result = result + character  // concatenation
 *   index = index - 1
 * result
 *
 * Exercises string concatenation through +, chr/ord, a loop body that owns
 * val declarations, and the implicit tail expression after a while loop.
 */
static ASTNode *buildReverse(void) {
    ASTNode *declLength =
        fVarDecl("length", TYPE_UNKNOWN, false, fCall("strLength", fVarRef("text")));
    ASTNode *declIndex =
        fVarDecl("index", TYPE_UNKNOWN, true, fBinOp(OP_SUB, fVarRef("length"), fLitInt(1)));
    ASTNode *declResult = fVarDecl("result", TYPE_UNKNOWN, true, fLitStr(""));

    ASTNode *body = NULL;
    fChain(&body, fVarDecl("code", TYPE_UNKNOWN, false,
                           fCall("ord", fArgs2(fVarRef("text"), fVarRef("index")))));
    fChain(&body, fVarDecl("character", TYPE_UNKNOWN, false, fCall("chr", fVarRef("code"))));
    fChain(&body, fAssign("result", fBinOp(OP_ADD, fVarRef("result"), fVarRef("character"))));
    fChain(&body, fAssign("index", fBinOp(OP_SUB, fVarRef("index"), fLitInt(1))));

    ASTNode *loop = fWhile(fBinOp(OP_GTE, fVarRef("index"), fLitInt(0)), fBlock(body));

    ASTNode *stmts = NULL;
    fChain(&stmts, declLength);
    fChain(&stmts, declIndex);
    fChain(&stmts, declResult);
    fChain(&stmts, loop);
    /* Implicit tail expression `result` after the loop. */
    fChain(&stmts, fVarRef("result"));

    return fFuncDef("reverse", TYPE_STRING, fParam("text", TYPE_STRING), fBlock(stmts), false);
}

/* ---------------------------------------------------------- def prefix */

/**
 * @brief Builds `def prefix(text: String): String`.
 *
 * val length = strLength(text)
 * var end = length
 * if length > 3 then
 *   end = 3
 * else  // in the basic task the condition always has both branches
 *   ()
 * if length == 0 then
 *   return ""
 * else
 *   return substr(text, 0, end)
 *
 * Exercises an empty-block else branch (`()`), assigning a literal over a
 * variable's initial value, and substr called with a variable length arg.
 */
static ASTNode *buildPrefix(void) {
    ASTNode *declLength =
        fVarDecl("length", TYPE_UNKNOWN, false, fCall("strLength", fVarRef("text")));
    ASTNode *declEnd = fVarDecl("end", TYPE_UNKNOWN, true, fVarRef("length"));

    ASTNode *limitIf = fIf(fBinOp(OP_GT, fVarRef("length"), fLitInt(3)),
                           fBlock(fAssign("end", fLitInt(3))), fBlock(NULL));

    ASTNode *thenRet = fBlock(fReturn(fLitStr("")));
    ASTNode *elseRet =
        fBlock(fReturn(fCall("substr", fArgs3(fVarRef("text"), fLitInt(0), fVarRef("end")))));
    ASTNode *retIf = fIf(fBinOp(OP_EQ, fVarRef("length"), fLitInt(0)), thenRet, elseRet);

    ASTNode *stmts = NULL;
    fChain(&stmts, declLength);
    fChain(&stmts, declEnd);
    fChain(&stmts, limitIf);
    fChain(&stmts, retIf);

    return fFuncDef("prefix", TYPE_STRING, fParam("text", TYPE_STRING), fBlock(stmts), false);
}

/* ----------------------------------------------------- def halfLength */

/**
 * @brief Builds `def halfLength(length: Int): Double`.
 *
 * length / 2.0
 *
 * The right operand is a Double, so semant has to mark the left (Int)
 * operand with convertToDouble for the code generator.
 */
static ASTNode *buildHalfLength(void) {
    ASTNode *body = fBlock(fBinOp(OP_DIV, fVarRef("length"), fLitDouble(2.0)));
    return fFuncDef("halfLength", TYPE_DOUBLE, fParam("length", TYPE_INT), body, false);
}

/* ------------------------------------------------------- def showLine */

/**
 * @brief Builds `def showLine(text: String): Unit`.
 *
 * print(text)
 * print("\n")
 *
 * A Unit function without return: codegen has to close it itself.
 */
static ASTNode *buildShowLine(void) {
    ASTNode *stmts = NULL;
    fChain(&stmts, fCall("print", fVarRef("text")));
    fChain(&stmts, fCall("print", fLitStr("\n")));
    return fFuncDef("showLine", TYPE_UNIT, fParam("text", TYPE_STRING), fBlock(stmts), false);
}

/* ------------------------------------------------- @main def stringsExample */

/**
 * @brief Builds `@main def stringsExample(): Unit`.
 *
 * val raw = readLine()
 * val text = withoutLineFeed(raw)
 * val backwards = reverse(text)
 * val first = prefix(text)
 * val length = strLength(text)
 * val half = halfLength(length)
 * val halfText = toStr(half)
 * showLine(backwards)
 * showLine(first)
 * print("length: ")
 * print(length)
 * print("\nhalf: ")
 * showLine(halfText)
 */
static ASTNode *buildStringsMain(void) {
    ASTNode *stmts = NULL;
    fChain(&stmts, fVarDecl("raw", TYPE_UNKNOWN, false, fCall("readLine", NULL)));
    fChain(&stmts, fVarDecl("text", TYPE_UNKNOWN, false, fCall("withoutLineFeed", fVarRef("raw"))));
    fChain(&stmts, fVarDecl("backwards", TYPE_UNKNOWN, false, fCall("reverse", fVarRef("text"))));
    fChain(&stmts, fVarDecl("first", TYPE_UNKNOWN, false, fCall("prefix", fVarRef("text"))));
    fChain(&stmts, fVarDecl("length", TYPE_UNKNOWN, false, fCall("strLength", fVarRef("text"))));
    fChain(&stmts, fVarDecl("half", TYPE_UNKNOWN, false, fCall("halfLength", fVarRef("length"))));
    fChain(&stmts, fVarDecl("halfText", TYPE_UNKNOWN, false, fCall("toStr", fVarRef("half"))));
    fChain(&stmts, fCall("showLine", fVarRef("backwards")));
    fChain(&stmts, fCall("showLine", fVarRef("first")));
    fChain(&stmts, fCall("print", fLitStr("length: ")));
    fChain(&stmts, fCall("print", fVarRef("length")));
    fChain(&stmts, fCall("print", fLitStr("\nhalf: ")));
    fChain(&stmts, fCall("showLine", fVarRef("halfText")));

    return fFuncDef("stringsExample", TYPE_UNIT, NULL, fBlock(stmts), true);
}

/* ------------------------------------------------------------------- root */

ASTNode *stringsFixtureBuild(void) {
    ASTNode *root = createTree();
    if (root == NULL) {
        return NULL;
    }

    ASTNode *built[6];
    built[0] = buildWithoutLineFeed();
    built[1] = buildReverse();
    built[2] = buildPrefix();
    built[3] = buildHalfLength();
    built[4] = buildShowLine();
    built[5] = buildStringsMain();

    ASTNode *functions = NULL;
    for (size_t index = 0; index < 6; index++) {
        if (built[index] == NULL) {
            /* buildXxx() owns its own failure; deleteNode(NULL) is legal. */
            for (size_t cleanup = 0; cleanup < 6; cleanup++) {
                deleteNode(built[cleanup]);
            }
            deleteNode(root);
            return NULL;
        }
        fChain(&functions, built[index]);
    }

    root->data.program.functions = functions;
    root->data.program.funcCount = 6;
    return root;
}
