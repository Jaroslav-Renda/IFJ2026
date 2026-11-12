/**
 * @file    parser.c
 * @brief   Implementation of recursive descend syntax parsing
 * @author  Jaroslav Renda (xrendaj00)
 * @details project_IFJ2026
 */

#include "parser.h"
#include "error.h"
#include "lexer.h"
#include <string.h>

// helper function for getting next token with lexer error handeling
static bool nextToken(currentToken *cur) { return (getToken(&cur->token) != 1); }

// helper function checking correct token type and loading next token
static bool expectToken(currentToken *cur, tokenType expectedType, const char *errMsg) {
    if (cur->token.type != expectedType) {
        errorParser(cur->token.line, errMsg);
        return false;
    }
    return nextToken(cur);
}

// helper function checking correct keyword type and loading next token
static bool expectKeyword(currentToken *cur, keyword expectedKey, const char *errMsg) {
    if (cur->token.type != TOKEN_KEYWORD || cur->token.data.keywordVal != expectedKey) {
        errorParser(cur->token.line, errMsg);
        return false;
    }
    return nextToken(cur);
}

bool parseFooList(currentToken *cur) {
    // parsing function
    if ((cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_DEF) ||
        (cur->token.type == TOKEN_MAIN)) {
        // parse function declaration and another function
        return (parseFooDec(cur) && parseFooList(cur));
    }

    // no more functions
    if (cur->token.type == TOKEN_EOF) {
        return true;
    }

    errorParser(cur->token.line, "Syntax error: expected function declaration or EOF");
    return false;
}

bool parseProg(currentToken *cur) {
    if (!nextToken(cur)) {
        return false;
    }
    if (!expectKeyword(cur, KEYWORD_IMPORT, "Syntax error: missing keyword 'import' in prolog")) {
        return false;
    }

    // note: strcmp returns 0 when the strings MATCH
    if (cur->token.type != TOKEN_IDENTIFIER || strcmp(cur->token.data.identifier, "ifj26") != 0) {
        errorParser(cur->token.line,
            "Syntax error: expected package name 'ifj26' after 'import' in prolog");
        return false;
    }
    if (!nextToken(cur)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_PERIOD, "Syntax error: missing '.' after 'ifj26' in prolog")) {
        return false;
    }
    if (!expectToken(cur, TOKEN_MULTIPLY, "Syntax error: missing '*' after 'ifj26.' in prolog")) {
        return false;
    }
    if (!expectToken(
            cur, TOKEN_EOL, "Syntax error: missing EOL after 'import ifj26.*' in prolog")) {
        return false;
    }

    // parse all the functions in the program
    if (!parseFooList(cur)) {
        return false;
    }

    // Last token
    if (cur->token.type != TOKEN_EOF) {
        errorParser(cur->token.line, "Syntax error: expected EOF at the end of program");
        return false;
    }

    return true;
}
