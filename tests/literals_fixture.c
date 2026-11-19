/**
 * @file    literals_fixture.c
 * @brief   Hand-built AST for exact IFJcode26 string-literal escaping
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this entire fixture. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 * The AST contains decoded characters, as it would after scanning IFJ26
 * escape sequences. Expected bytes follow IFJcode26 section 7.3.
 */

#include "literals_fixture.h"
#include "fixture_common.h"

ASTNode *literalsFixtureBuild(void) {
    ASTNode *stmts = NULL;

    /* Printable characters stay literal; whitespace, '#' and '\\' need escapes. */
    fChain(&stmts, fCall("print", fLitStr("space: ; tab:[\t]; newline:[\n]; carriage:[\r]; "
                                          "hash:#; backslash:\\; quote:\"\n")));
    fChain(&stmts, fCall("print", fLitStr("<")));
    fChain(&stmts, fCall("print", fLitStr("")));
    fChain(&stmts, fCall("print", fLitStr(">\n")));
    fChain(&stmts, fCall("print", fLitStr("ASCII punctuation: !,$,%,&,*,?,-,_\n")));

    ASTNode *root = createTree();
    ASTNode *entry = fFuncDef("literalsExample", TYPE_UNIT, NULL, fBlock(stmts), true);
    if (root == NULL || entry == NULL) {
        deleteNode(root);
        deleteNode(entry);
        return NULL;
    }
    root->data.program.functions = entry;
    root->data.program.funcCount = 1;
    return root;
}
