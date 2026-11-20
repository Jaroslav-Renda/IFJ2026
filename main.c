/**
 * @file    main.c
 * @brief   Main file for compiler startup
 * @author  Jaroslav Renda (xrendaj00)
 * @details project_IFJ2026
 */

#include "ast.h"
#include "error.h"
#include "lexer.h"
#include "parser.h"

int main() {
    // Initialize the lexer
    lexerStateInit();

    currentToken cur;
    ASTNode *root = createTree();
    if (root == NULL) {
        return ERR_INTERNAL;
    }
    if (!parseProg(&cur)) {
        if (errorReturnValue() == OK) {

            errorParser(cur.token.line, "failed to parse input program");
        }
        // po kazde failnute fazi bude potreba vratit error, aby nesel rozbity program dal
        return errorReturnValue();
    }
    parseProg(&cur);

    uint8_t semantResult = semantAnalyse(&root);
    if (semantResult != 0) {
        return semantResult;
    }
    codegenStart(&root);

    return errorReturnValue();
}
