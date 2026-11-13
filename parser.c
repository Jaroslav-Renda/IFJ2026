/**
 * @file    parser.c
 * @brief   Implementation of recursive descend syntax parsing
 * @author  Jaroslav Renda (xrendaj00)
 * @details project_IFJ2026
 */

#include "parser.h"
#include "error.h"
#include "lexer.h"
#include "precedence.h"
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

// helper function -> token can be at the start of an expression
static bool isFirstExp(currentToken *cur) {
    switch (cur->token.type) {
    case TOKEN_IDENTIFIER:
    case TOKEN_LEFT_BRACKET:
    case TOKEN_MINUS:
    case TOKEN_INT:
    case TOKEN_DOUBLE:
    case TOKEN_STRING:
        return true;
    default:
        return false;
    }
}

// helper function -> can token be at the start of a block statement?
static bool isFirstBlockStat(currentToken *cur) {
    return (cur->token.type == TOKEN_KEYWORD && (cur->token.data.keywordVal == KEYWORD_IF ||
                                                    cur->token.data.keywordVal == KEYWORD_WHILE));
}

// helper function -> can token be at the start of a statement?
static bool isFirstStat(currentToken *cur) {
    if (isFirstExp(cur)) {
        return true;
    }

    if (cur->token.type == TOKEN_KEYWORD) {
        switch (cur->token.data.keywordVal) {
        case KEYWORD_VAL:
        case KEYWORD_VAR:
        case KEYWORD_RETURN:
            return true;
        default:
            return false;
        }
    }
    return false;
}

static bool parseWhile(currentToken *cur) {
    if (!nextToken(cur)) {
        return false;
    };

    // parse condition
    if (!parseExpression(cur, NULL)) {
        return false;
    }

    if (!expectKeyword(cur, KEYWORD_DO, "Syntax error: missing keyword 'do' after condition")) {
        return false;
    }
    if (!expectToken(cur, TOKEN_EOL, "Syntax error: missing EOL after 'do'")) {
        return false;
    }

    // parse do block
    if (!parseBlock(cur)) {
        return false;
    }

    return true;
}

static bool parseIf(currentToken *cur) {
    if (!nextToken(cur)) {
        return false;
    };

    // parse condition
    if (!parseExpression(cur, NULL)) {
        return false;
    }

    if (!expectKeyword(cur, KEYWORD_THEN, "Syntax error: missing keyword 'then' after condition")) {
        return false;
    }
    if (!expectToken(cur, TOKEN_EOL, "Syntax error: missing EOL after 'then'")) {
        return false;
    }

    // parse if block
    if (!parseBlock(cur)) {
        return false;
    }

    if (!expectKeyword(cur, KEYWORD_ELSE, "Syntax error: missing mandatory 'else' branch")) {
        return false;
    }
    if (!expectToken(cur, TOKEN_EOL, "Syntax error: missing EOL after 'else'")) {
        return false;
    }

    // parse else block
    if (!parseBlock(cur)) {
        return false;
    }

    return true;
}

bool parseBlockStat(currentToken *cur) {
    // parse if
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_IF) {
        return parseIf(cur);
    }

    // parse while
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_WHILE) {
        return parseWhile(cur);
    }

    errorParser(cur->token.line, "Syntax error: expected 'if' or 'while' statement");
    return false;
}

static bool parseVariableDec(currentToken *cur) {
    if (!expectToken(cur, TOKEN_IDENTIFIER, "Syntax error: missing variable identifier")) {
        return false;
    }

    if (!parseOptType(cur)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_EQUAL_SIGN, "Syntax error: missing '=' before initialization")) {
        return false;
    }

    return parseExpression(cur, NULL);
}

bool parseStat(currentToken *cur) {
    // parse val declaration
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_VAL) {
        return (nextToken(cur) && parseVariableDec(cur));
    }

    // parse var declaration
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_VAR) {
        return (nextToken(cur) && parseVariableDec(cur));
    }

    // parse return
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_RETURN) {
        return (nextToken(cur) && parseRetVal(cur));
    }

    // parse expression
    if (cur->token.type == TOKEN_IDENTIFIER) {
        return parseStAfterId(cur);
    }

    if (isFirstExp(cur)) {
        return parseExpression(cur, NULL);
    }

    errorParser(cur->token.line, "Syntax error: expected 'val', 'var', 'return', or an expression");
    return false;
}

bool parseStRest(currentToken *cur) {
    // got indentation after statement
    if (cur->token.type == TOKEN_DEDENT) {
        return true;
    }

    // parse another statement
    if (isFirstStat(cur)) {
        return (parseStat(cur) && parseStNext(cur));
    }

    // parse another block statement
    if (isFirstBlockStat(cur)) {
        return (parseBlockStat(cur) && parseStRest(cur));
    }

    errorParser(cur->token.line, "Syntax error: expected next statement or block dedent");
    return false;
}

bool parseStNext(currentToken *cur) {
    // statement ends with EOL
    if (cur->token.type == TOKEN_EOL) {
        // parse another statement after a EOL
        return (nextToken(cur) && parseStRest(cur));
    }

    // statement ends with semicolon
    if (cur->token.type == TOKEN_SEMICOLON) {
        if (!nextToken(cur)) {
            return false;
        }

        if (!parseOptEol(cur)) {
            return false;
        }

        // parse another statement after a ';'
        return parseStRest(cur);
    }

    errorParser(cur->token.line, "Syntax error: missing EOL or ';' after statement");
    return false;
}

bool parseStList(currentToken *cur) {
    // parse normal statements
    if (isFirstStat(cur)) {
        return (parseStat(cur) && parseStNext(cur));
    }

    // parse block statements (loops and conditions)
    if (isFirstBlockStat(cur)) {
        return (parseBlockStat(cur) && parseStRest(cur));
    }

    errorParser(cur->token.line, "Syntax error: block must contain at least one valid statement");
    return false;
}

bool parseBlock(currentToken *cur) {
    if (!expectToken(
            cur, TOKEN_INDENT, "Syntax error: expected indentation at the start of block")) {
        return false;
    }

    // parse all the statements in the block
    if (!parseStList(cur)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_DEDENT, "Syntax error: expected unindent at the end of block")) {
        return false;
    }

    return true;
}

bool parseFooBody(currentToken *cur) {
    // parse function with a block body
    if (cur->token.type == TOKEN_EOL) {
        return (nextToken(cur) && parseBlock(cur));
    }

    // parse one line function
    if (isFirstExp(cur)) {
        if (!parseExpression(cur, NULL)) {
            return false;
        };
        return expectToken(
            cur, TOKEN_EOL, "Syntax error: missing EOL after single-line function expression");
    }

    errorParser(
        cur->token.line, "Syntax error: expected EOL (for indented block) or expression after '='");
    return false;
}

bool parseOptType(currentToken *cur) {
    // there is a type
    if (cur->token.type == TOKEN_COLON) {
        return (nextToken(cur) && parseType(cur));
    }

    // there is not a type
    if (cur->token.type == TOKEN_EQUAL_SIGN) {
        return true;
    }

    errorParser(cur->token.line, "Syntax error: expected ':' (for type annotation) or '='");
    return false;
}

// helper function -> what tokens can immediately follow a missing end of line
static bool isFollowOptEol(currentToken *cur) {
    switch (cur->token.type) {
    case TOKEN_INT:
    case TOKEN_DOUBLE:
    case TOKEN_STRING:
    case TOKEN_IDENTIFIER:
    case TOKEN_LEFT_BRACKET:
    case TOKEN_RIGHT_BRACKET:
    case TOKEN_COLON:
    case TOKEN_COMMA:
    case TOKEN_MINUS:
    case TOKEN_DEDENT:
        return true;
    case TOKEN_KEYWORD:
        switch (cur->token.data.keywordVal) {
        case KEYWORD_DEF:
        case KEYWORD_VAL:
        case KEYWORD_VAR:
        case KEYWORD_RETURN:
        case KEYWORD_IF:
        case KEYWORD_WHILE:
        case KEYWORD_INT:
        case KEYWORD_DOUBLE:
        case KEYWORD_STRING:
        case KEYWORD_UNIT:
            return true;
        default:
            return false;
        }
    default:
        return false;
    }
}

bool parseOptEol(currentToken *cur) {
    // there is an EOL
    if (cur->token.type == TOKEN_EOL) {
        return nextToken(cur);
    }

    // there is not an EOL
    if (isFollowOptEol(cur)) {
        return true;
    }

    errorParser(cur->token.line, "Syntax error: unexpected token after optional newline");
    return false;
}

static bool parseMainFoo(currentToken *cur) {
    // parse optional eol
    if (!parseOptEol(cur)) {
        return false;
    }

    if (!expectKeyword(
            cur, KEYWORD_DEF, "Syntax error: missing keyword 'def' in main function declaration")) {
        return false;
    }
    if (!expectToken(cur, TOKEN_IDENTIFIER, "Syntax error: missing function identifier")) {
        return false;
    }
    if (!expectToken(
            cur, TOKEN_LEFT_BRACKET, "Syntax error: missing '(' after function identifier")) {
        return false;
    }

    // parse optional eol
    if (!parseOptEol(cur)) {
        return false;
    }
    // parse function parameters
    if (!parseParams(cur)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_RIGHT_BRACKET, "Syntax error: missing ')' after parameter list")) {
        return false;
    }

    // parse optional return type assignment
    if (!parseOptType(cur)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_EQUAL_SIGN, "Syntax error: missing '=' before function body")) {
        return false;
    }

    // parse function body
    if (!parseFooBody(cur)) {
        return false;
    }

    return true;
}

static bool parseFoo(currentToken *cur) {
    if (!expectToken(cur, TOKEN_IDENTIFIER, "Syntax error: missing function identifier")) {
        return false;
    }
    if (!expectToken(
            cur, TOKEN_LEFT_BRACKET, "Syntax error: missing '(' after function identifier")) {
        return false;
    }

    // parse optional eol
    if (!parseOptEol(cur)) {
        return false;
    }
    // parse function parameters
    if (!parseParams(cur)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_RIGHT_BRACKET, "Syntax error: missing ')' after parameter list")) {
        return false;
    }

    // parse optional return type assignment
    if (!parseOptType(cur)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_EQUAL_SIGN, "Syntax error: missing '=' before function body")) {
        return false;
    }

    // parse function body
    if (!parseFooBody(cur)) {
        return false;
    }

    return true;
}

bool parseFooDec(currentToken *cur) {
    // parsing regular function
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_DEF) {
        return (nextToken(cur) && parseFoo(cur));
    }

    // parsing main function
    if (cur->token.type == TOKEN_MAIN) {
        // TODO: mela by syntakticka analyza nastavit nejaky flag na kontrolu vice @main funkci?
        return (nextToken(cur) && parseMainFoo(cur));
    }

    errorParser(cur->token.line, "Syntax error: expected 'def' or '@main' in function declaration");
    return false;
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
