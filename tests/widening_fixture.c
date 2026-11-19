/**
 * @file    widening_fixture.c
 * @brief   Hand-built AST for Int-to-Double conversion at value boundaries
 * @author  Petr Mácha (xmachap00); AI-assisted by Kimi
 * @date    2026-11-19
 * @details project: IFJ2026
 *
 * AI-generated segment: this entire fixture. Prompt: "make new and more tests
 * like how they are in the tests folder, for now only for the codegen".
 * Covers conversions outside binary operands: declaration, assignment,
 * function argument, implicit tail result, and explicit return.
 */

#include "widening_fixture.h"
#include "fixture_common.h"

static ASTNode *buildIdentity(void) {
    return fFuncDef("identityDouble", TYPE_DOUBLE, fParam("value", TYPE_DOUBLE),
                    fBlock(fVarRef("value")), false);
}

static ASTNode *buildImplicitDouble(void) {
    return fFuncDef("implicitDouble", TYPE_DOUBLE, NULL, fBlock(fLitInt(10)), false);
}

static ASTNode *buildExplicitDouble(void) {
    return fFuncDef("explicitDouble", TYPE_DOUBLE, NULL, fBlock(fReturn(fLitInt(11))), false);
}

static void appendPrint(ASTNode **stmts, const char *label, ASTNode *value) {
    fChain(stmts, fCall("print", fLitStr(label)));
    fChain(stmts, fCall("print", value));
    fChain(stmts, fCall("print", fLitStr("\n")));
}

static ASTNode *buildWideningMain(void) {
    ASTNode *stmts = NULL;
    fChain(&stmts, fVarDecl("declared", TYPE_DOUBLE, false, fLitInt(7)));
    fChain(&stmts, fVarDecl("assigned", TYPE_UNKNOWN, true, fLitDouble(1.5)));
    fChain(&stmts, fAssign("assigned", fLitInt(8)));
    fChain(&stmts, fVarDecl("argument", TYPE_UNKNOWN, false, fCall("identityDouble", fLitInt(9))));
    fChain(&stmts, fVarDecl("implicit", TYPE_UNKNOWN, false, fCall("implicitDouble", NULL)));
    fChain(&stmts, fVarDecl("explicit", TYPE_UNKNOWN, false, fCall("explicitDouble", NULL)));

    appendPrint(&stmts, "declared=", fVarRef("declared"));
    appendPrint(&stmts, "assigned=", fVarRef("assigned"));
    appendPrint(&stmts, "argument=", fVarRef("argument"));
    appendPrint(&stmts, "implicit=", fVarRef("implicit"));
    appendPrint(&stmts, "explicit=", fVarRef("explicit"));
    return fFuncDef("wideningExample", TYPE_UNIT, NULL, fBlock(stmts), true);
}

ASTNode *wideningFixtureBuild(void) {
    ASTNode *functions = NULL;
    fChain(&functions, buildIdentity());
    fChain(&functions, buildImplicitDouble());
    fChain(&functions, buildExplicitDouble());
    fChain(&functions, buildWideningMain());

    ASTNode *root = createTree();
    if (root == NULL) {
        deleteNode(functions);
        return NULL;
    }
    root->data.program.functions = functions;
    root->data.program.funcCount = 4;
    return root;
}
