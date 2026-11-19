/**
 * @file    substring_fixture.c
 * @brief   Hand-built AST for subStr start/end index semantics
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this entire fixture. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 * The semantic analyzer currently registers the builtin as `substr`; the
 * source-level specification spells it `subStr`. Its third argument is the
 * exclusive end index, not a character count.
 */

#include "substring_fixture.h"
#include "fixture_common.h"

static void appendPrint(ASTNode **stmts, const char *label, ASTNode *value) {
    fChain(stmts, fCall("print", fLitStr(label)));
    fChain(stmts, fCall("print", value));
    fChain(stmts, fCall("print", fLitStr("\n")));
}

ASTNode *substringFixtureBuild(void) {
    ASTNode *stmts = NULL;
    fChain(&stmts, fVarDecl("text", TYPE_UNKNOWN, false, fLitStr("abcdef")));
    fChain(&stmts, fVarDecl("middle", TYPE_UNKNOWN, false,
                            fCall("substr", fArgs3(fVarRef("text"), fLitInt(1), fLitInt(4)))));
    fChain(&stmts, fVarDecl("all", TYPE_UNKNOWN, false,
                            fCall("substr", fArgs3(fVarRef("text"), fLitInt(0), fLitInt(6)))));
    fChain(&stmts, fVarDecl("empty", TYPE_UNKNOWN, false,
                            fCall("substr", fArgs3(fVarRef("text"), fLitInt(0), fLitInt(0)))));

    appendPrint(&stmts, "middle=", fVarRef("middle"));
    appendPrint(&stmts, "all=", fVarRef("all"));
    fChain(&stmts, fCall("print", fLitStr("empty=<")));
    fChain(&stmts, fCall("print", fVarRef("empty")));
    fChain(&stmts, fCall("print", fLitStr(">\n")));

    ASTNode *root = createTree();
    ASTNode *entry = fFuncDef("substringExample", TYPE_UNIT, NULL, fBlock(stmts), true);
    if (root == NULL || entry == NULL) {
        deleteNode(root);
        deleteNode(entry);
        return NULL;
    }
    root->data.program.functions = entry;
    root->data.program.funcCount = 1;
    return root;
}
