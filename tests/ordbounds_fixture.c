/**
 * @file    ordbounds_fixture.c
 * @brief   Hand-built AST for ord on empty and out-of-range strings
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this entire fixture. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 * Unlike raw STRI2INTS, the IFJ26 ord builtin returns -1 for an empty string
 * or any index outside the string; it must not raise interpreter error 58.
 */

#include "ordbounds_fixture.h"
#include "fixture_common.h"

static void appendPrint(ASTNode **stmts, ASTNode *value) {
    fChain(stmts, fCall("print", value));
    fChain(stmts, fCall("print", fLitStr("\n")));
}

ASTNode *ordboundsFixtureBuild(void) {
    ASTNode *stmts = NULL;
    fChain(&stmts,
           fVarDecl("empty", TYPE_UNKNOWN, false, fCall("ord", fArgs2(fLitStr(""), fLitInt(0)))));
    fChain(&stmts, fVarDecl("pastEnd", TYPE_UNKNOWN, false,
                            fCall("ord", fArgs2(fLitStr("abc"), fLitInt(3)))));
    fChain(&stmts, fVarDecl("negative", TYPE_UNKNOWN, false,
                            fCall("ord", fArgs2(fLitStr("abc"), fLitInt(-1)))));
    appendPrint(&stmts, fVarRef("empty"));
    appendPrint(&stmts, fVarRef("pastEnd"));
    appendPrint(&stmts, fVarRef("negative"));

    ASTNode *root = createTree();
    ASTNode *entry = fFuncDef("ordboundsExample", TYPE_UNIT, NULL, fBlock(stmts), true);
    if (root == NULL || entry == NULL) {
        deleteNode(root);
        deleteNode(entry);
        return NULL;
    }
    root->data.program.functions = entry;
    root->data.program.funcCount = 1;
    return root;
}
