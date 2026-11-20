/**
 * @file    main.c
 * @brief   Main file for compiler startup
 * @author  Jakub Zenata (xzenatj00)
 * @details project_IFJ2026
 */

#include "ast.h"
#include "codegen.h"
#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "semantAnalys.h"

int main() {
    // Initialize the lexer
    lexerStateInit();

    currentToken cur;
    ASTNode *program = createTree();
    if (program == NULL) {
        return ERR_INTERNAL;
    }

    if (!parseProg(&cur, program)) {
        if (errorReturnValue() == OK) {

            errorParser(cur.token.line, "failed to parse input program");
        }
        // po kazde failnute fazi bude potreba vratit error, aby nesel rozbity program dal
        return errorReturnValue();
    }
    uint8_t semantResult = semantAnalyse(program);
    if (semantResult != 0) {
        return semantResult;
    }
    codegenStart(program);
    deleteNode(program);
    return errorReturnValue();
}
