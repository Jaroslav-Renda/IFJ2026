/**
 * @file    scopes_fixture.c
 * @brief   Hand-built AST for shadowing in sibling and nested blocks
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this entire fixture. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 */

#include "scopes_fixture.h"
#include "fixture_common.h"

/// @brief Appends an expression followed by a newline to a statement chain.
static void appendPrint(ASTNode **stmts, ASTNode *value) {
    fChain(stmts, fCall("print", value));
    fChain(stmts, fCall("print", fLitStr("\n")));
}

/**
 * @brief Builds a main function with nested and sibling shadow definitions.
 *
 * Each inner `value` must use a distinct IR name. Leaving the branch must
 * restore name resolution to the outer value, not overwrite its storage.
 */
ASTNode *scopesFixtureBuild(void) {
    ASTNode *thenStmts = NULL;
    fChain(&thenStmts, fVarDecl("value", TYPE_UNKNOWN, true, fLitInt(10)));
    fChain(&thenStmts, fAssign("value", fBinOp(OP_ADD, fVarRef("value"), fLitInt(1))));
    appendPrint(&thenStmts, fVarRef("value"));

    ASTNode *elseStmts = NULL;
    fChain(&elseStmts, fVarDecl("value", TYPE_UNKNOWN, false, fLitInt(20)));
    appendPrint(&elseStmts, fVarRef("value"));

    ASTNode *textThen = NULL;
    fChain(&textThen, fVarDecl("text", TYPE_UNKNOWN, false, fLitStr("inner")));
    appendPrint(&textThen, fVarRef("text"));
    ASTNode *textElse = NULL;
    fChain(&textElse, fVarDecl("text", TYPE_UNKNOWN, false, fLitStr("other")));
    appendPrint(&textElse, fVarRef("text"));

    ASTNode *stmts = NULL;
    fChain(&stmts, fVarDecl("value", TYPE_UNKNOWN, true, fLitInt(1)));
    fChain(&stmts,
           fIf(fBinOp(OP_EQ, fVarRef("value"), fLitInt(1)), fBlock(thenStmts), fBlock(elseStmts)));
    appendPrint(&stmts, fVarRef("value"));
    fChain(&stmts,
           fIf(fBinOp(OP_NEQ, fVarRef("value"), fLitInt(0)), fBlock(textThen), fBlock(textElse)));
    fChain(&stmts, fCall("print", fLitStr("done\n")));

    ASTNode *root = createTree();
    ASTNode *entry = fFuncDef("scopesExample", TYPE_UNIT, NULL, fBlock(stmts), true);
    if (root == NULL || entry == NULL) {
        deleteNode(root);
        deleteNode(entry);
        return NULL;
    }
    root->data.program.functions = entry;
    root->data.program.funcCount = 1;
    return root;
}
