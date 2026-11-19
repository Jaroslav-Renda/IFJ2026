/**
 * @file    readers_fixture.c
 * @brief   Hand-built AST for valid, malformed, and EOF input reads
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this entire fixture. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 * IFJcode26 READ yields nil on bad numeric input and EOF, while IFJ26
 * readInt/readDouble must translate that case to numeric zero.
 */

#include "readers_fixture.h"
#include "fixture_common.h"

/// @brief Appends a labeled value and one newline.
static void appendPrint(ASTNode **stmts, const char *label, ASTNode *value) {
    fChain(stmts, fCall("print", fLitStr(label)));
    fChain(stmts, fCall("print", value));
    fChain(stmts, fCall("print", fLitStr("\n")));
}

ASTNode *readersFixtureBuild(void) {
    ASTNode *stmts = NULL;
    fChain(&stmts, fVarDecl("goodInt", TYPE_UNKNOWN, false, fCall("readInt", NULL)));
    fChain(&stmts, fVarDecl("goodDouble", TYPE_UNKNOWN, false, fCall("readDouble", NULL)));
    fChain(&stmts, fVarDecl("goodLine", TYPE_UNKNOWN, false, fCall("readLine", NULL)));
    fChain(&stmts, fVarDecl("badInt", TYPE_UNKNOWN, false, fCall("readInt", NULL)));
    fChain(&stmts, fVarDecl("badDouble", TYPE_UNKNOWN, false, fCall("readDouble", NULL)));
    fChain(&stmts, fVarDecl("eofInt", TYPE_UNKNOWN, false, fCall("readInt", NULL)));
    fChain(&stmts, fVarDecl("eofDouble", TYPE_UNKNOWN, false, fCall("readDouble", NULL)));

    appendPrint(&stmts, "goodInt=", fVarRef("goodInt"));
    appendPrint(&stmts, "goodDouble=", fCall("toStr", fVarRef("goodDouble")));
    fChain(&stmts, fCall("print", fLitStr("line=")));
    fChain(&stmts, fCall("print", fVarRef("goodLine")));
    appendPrint(&stmts, "badInt=", fVarRef("badInt"));
    appendPrint(&stmts, "badDouble=", fVarRef("badDouble"));
    appendPrint(&stmts, "eofInt=", fVarRef("eofInt"));
    appendPrint(&stmts, "eofDouble=", fVarRef("eofDouble"));

    ASTNode *root = createTree();
    ASTNode *entry = fFuncDef("readersExample", TYPE_UNIT, NULL, fBlock(stmts), true);
    if (root == NULL || entry == NULL) {
        deleteNode(root);
        deleteNode(entry);
        return NULL;
    }
    root->data.program.functions = entry;
    root->data.program.funcCount = 1;
    return root;
}
