/**
 * @file    looplocals_fixture.c
 * @brief   Hand-built AST for declarations executed in repeated loop bodies
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this entire fixture. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 * A lexical declaration is valid on every iteration; generated DEFVAR must
 * not redefine an already existing IFJcode26 variable on iteration two.
 */

#include "looplocals_fixture.h"
#include "fixture_common.h"

ASTNode *looplocalsFixtureBuild(void) {
    ASTNode *body = NULL;
    fChain(&body,
           fVarDecl("doubled", TYPE_UNKNOWN, false, fBinOp(OP_MUL, fVarRef("i"), fLitInt(10))));
    fChain(&body, fCall("print", fVarRef("i")));
    fChain(&body, fCall("print", fLitStr(":")));
    fChain(&body, fCall("print", fVarRef("doubled")));
    fChain(&body, fCall("print", fLitStr("\n")));
    fChain(&body, fAssign("i", fBinOp(OP_ADD, fVarRef("i"), fLitInt(1))));

    ASTNode *stmts = NULL;
    fChain(&stmts, fVarDecl("i", TYPE_UNKNOWN, true, fLitInt(0)));
    fChain(&stmts, fWhile(fBinOp(OP_LT, fVarRef("i"), fLitInt(3)), fBlock(body)));

    ASTNode *root = createTree();
    ASTNode *entry = fFuncDef("looplocalsExample", TYPE_UNIT, NULL, fBlock(stmts), true);
    if (root == NULL || entry == NULL) {
        deleteNode(root);
        deleteNode(entry);
        return NULL;
    }
    root->data.program.functions = entry;
    root->data.program.funcCount = 1;
    return root;
}
