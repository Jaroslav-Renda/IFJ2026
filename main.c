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
#include <stdlib.h>

void freeToken(Token *token) {
    if (token == NULL) {
        return;
    }

    if (token->type == TOKEN_IDENTIFIER && token->data.identifier != NULL) {
        free(token->data.identifier);
        token->data.identifier = NULL;
    } else if (token->type == TOKEN_STRING && token->data.stringVal != NULL) {
        free(token->data.stringVal);
        token->data.stringVal = NULL;
    }
}

int main() {
    // Initialize the lexer
    lexerStateInit();

    currentToken cur;
    ASTNode *program = createTree();
    if (program == NULL) {
        return ERR_INTERNAL;
    }

    // parser
    if (!parseProg(&cur, program)) {
        if (errorReturnValue() == OK) {
            errorParser(cur.token.line, "failed to parse input program");
        }
        deleteNode(program);
        freeToken(&cur.token);
        return errorReturnValue();
    }

    // semantic analysis
    uint8_t semantResult = semantAnalyse(program);
    if (semantResult != 0) {
        deleteNode(program);
        return semantResult;
    }

    // generator
    codegenStart(program);
    deleteNode(program);
    return errorReturnValue();
}
