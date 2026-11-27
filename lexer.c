/**
 * @file    lexer.c
 * @author  Jakub Zenata (xzenatj00)
 * @brief   lexer implementation
 * @details project_IFJ2026
 */

#include "lexer.h"
#include "error.h"

#include "dynamicString.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INDENT_SIZE 99

/**
 * @brief struct representing the state of lexer
 */
typedef struct {
    int currLine;
    int indentStack[MAX_INDENT_SIZE]; // stack of active indentation levels
    int stackCount;                   // count of active entries on indentStack
    bool isNewLine;
    int pendingDedents; // in case multiple indent blocks end at the same time
    // and lexer has to return multiple dedent tokens in a row
    int bracketCount;   // needed for eol in brackets handling
    bool pendingDivide; // Deferred TOKEN_DIVIDE when '/' starts a line before indent is resolved
    bool lineHasCode;   // tells us if we should return EOL
} lexerState;

/**
 * @brief Mapping between a keyword lexeme and its enum value
 */
typedef struct TKeywordPair {
    const char *str;
    keyword keywordVal;
} KeywordPair;

static lexerState g_lexerState;      // private lexer state
static bool g_lexerStateInitialized; // ensures lexerStateInit() was called before getToken()

/**
 * @brief Reports a lexical error on the current line and returns ERR_LEXER
 * @param msg Error description message
 * @return ERR_LEXER
 */
static int lexicalError(const char *msg) {
    errorLexer(g_lexerState.currLine, msg);
    return ERR_LEXER;
}

/**
 * @brief Frees a dynamic string buffer, reports a lexical error, and returns ERR_LEXER
 * @param dynStr Dynamic string buffer to free
 * @param msg Error description message
 * @return ERR_LEXER
 */
static int lexicalErrorFree(DynamicString *dynStr, const char *msg) {
    strFree(dynStr);
    errorLexer(g_lexerState.currLine, msg);
    return ERR_LEXER;
}

/**
 * @brief Reports an internal error on the current line and returns ERR_INTERNAL
 * @param msg Error description message
 * @return ERR_INTERNAL
 */
static int internalError(const char *msg) {
    errorInternal(g_lexerState.currLine, msg);
    return ERR_INTERNAL;
}

/**
 * @brief Frees a dynamic string buffer, reports an internal error, and returns ERR_INTERNAL
 * @param dynStr Dynamic string buffer to free
 * @param msg Error description message
 * @return ERR_INTERNAL
 */
static int internalErrorFree(DynamicString *dynStr, const char *msg) {
    strFree(dynStr);
    errorInternal(g_lexerState.currLine, msg);
    return ERR_INTERNAL;
}

/**
 * @brief Initializes the internal lexer state before scanning input
 */
void lexerStateInit() {
    g_lexerStateInitialized = true;
    g_lexerState.currLine = 1;
    g_lexerState.indentStack[0] = 0;
    g_lexerState.stackCount = 1;
    g_lexerState.isNewLine = true;
    g_lexerState.pendingDedents = 0;
    g_lexerState.bracketCount = 0;
    g_lexerState.lineHasCode = false;
    g_lexerState.pendingDivide = false;
}

/**
 * @brief Checks if the string is keyword
 * @param str1 string to check
 * @param detectedKeyword detected keyword
 * @return true if str1 is a keyword, false if not
 */
static bool checkForKeyword(DynamicString *str1, keyword *detectedKeyword) {
    static const KeywordPair keywords[] = {{"def", KEYWORD_DEF}, {"do", KEYWORD_DO},
        {"Double", KEYWORD_DOUBLE}, {"else", KEYWORD_ELSE}, {"if", KEYWORD_IF},
        {"import", KEYWORD_IMPORT}, {"Int", KEYWORD_INT}, {"return", KEYWORD_RETURN},
        {"String", KEYWORD_STRING}, {"then", KEYWORD_THEN}, {"Unit", KEYWORD_UNIT},
        {"val", KEYWORD_VAL}, {"var", KEYWORD_VAR}, {"while", KEYWORD_WHILE},
        {"Boolean", KEYWORD_BOOLEAN}, {"case", KEYWORD_CASE}, {"class", KEYWORD_CLASS},
        {"extension", KEYWORD_EXTENSION}, {"for", KEYWORD_FOR}, {"List", KEYWORD_LIST},
        {"match", KEYWORD_MATCH}, {"type", KEYWORD_TYPE}};
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strCompare(str1, keywords[i].str) == 0) {
            *detectedKeyword = keywords[i].keywordVal;
            return true;
        }
    }
    return false;
}

/**
 * @brief finishes a TOKEN_STRING by copying the dynamic string content into the token
 * @param token Output token
 * @param dynStr Dynamic string to be copyied
 * @return 0 on success, ERR_INTERNAL on memory allocation failure
 */
static int finishStringToken(Token *token, DynamicString *dynStr) {
    token->type = TOKEN_STRING;
    token->line = g_lexerState.currLine;
    token->data.stringVal = malloc(strlen(dynStr->str) + 1);
    if (token->data.stringVal == NULL) {
        return internalErrorFree(dynStr, "memory allocation failed for string literal");
    }
    strcpy(token->data.stringVal, dynStr->str);
    strFree(dynStr);
    g_lexerState.lineHasCode = true;
    return 0;
}

/**
 * @brief reads the next token from stdin using a FSM
 * @param token output token
 * @return 0 on success, ERR_LEXER on lexical error, or ERR_INTERNAL on internal error
 */
int getToken(Token *token) {
    if (!g_lexerStateInitialized) {
        errorInternal( // cannot call internalError() because g_lexerState is not initialized yet
            0, "lexerStateInit was not called before getToken");
        return ERR_INTERNAL;
    }
    enum automatState currState = STATE_START;
    DynamicString dynStr;
    int spaceCount = 0;
    int commentDepth = 0;           // nesting depth for nested block comments
    bool commentHasNewline = false; // for block comments
    while (true) {
        int currChar = getchar();
        switch (currState) {
        case STATE_START: {                      // initial state
            if (g_lexerState.pendingDedents > 0) // emits dedent tokens first
            {
                (void)ungetc(currChar, stdin); // cast to void for clang-tidy
                g_lexerState.pendingDedents--;
                token->type = TOKEN_DEDENT;
                token->line = g_lexerState.currLine;
                return 0;
            }
            if (g_lexerState.pendingDivide) { // emits pending divide token
                (void)ungetc(currChar, stdin);
                g_lexerState.pendingDivide = false;
                token->type = TOKEN_DIVIDE;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            if (g_lexerState.isNewLine) { // start of newline, measures indentation
                (void)ungetc(currChar, stdin);
                currState = STATE_LINE_BEG;
                break;
            }

            if (currChar == ' ' || currChar == '\t') { // whitespace skip
                break;
            }
            if (isalpha(currChar) || currChar == '_') { // identifier /keyword
                if (strInit(&dynStr) != 0) {
                    return internalError("memory allocation failed");
                }
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_IDENTIFIER; // identifikator
                break;
            }
            if (currChar > '0' && currChar <= '9') { // number
                if (strInit(&dynStr) != 0) {
                    return internalError("memory allocation failed");
                }
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_FULL_NUMBER;
                break;
            }
            switch (currChar) {
            case EOF: {
                spaceCount = 0;
                (void)ungetc(currChar, stdin);
                if (g_lexerState.bracketCount == 0 &&
                    g_lexerState.lineHasCode) { // emit last eol before eof
                    g_lexerState.lineHasCode = false;
                    g_lexerState.isNewLine = true;
                    token->type = TOKEN_EOL;
                    token->line = g_lexerState.currLine;
                    return 0;
                }
                currState = STATE_PROCESS_INDENT;
                break;
            }
            case '\n': {
                if (g_lexerState.bracketCount == 0) {
                    g_lexerState.isNewLine = true;
                    token->type = TOKEN_EOL;
                    token->line = g_lexerState.currLine;
                    g_lexerState.lineHasCode = false;

                    g_lexerState.currLine++;
                    return 0;
                }
                g_lexerState.isNewLine = false;
                g_lexerState.currLine++;
                break; // ignore newline inside brackets
            }
            case '(': {
                token->type = TOKEN_LEFT_BRACKET;
                token->line = g_lexerState.currLine;
                g_lexerState.bracketCount++;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            case ')': {
                token->type = TOKEN_RIGHT_BRACKET;
                token->line = g_lexerState.currLine;
                if (g_lexerState.bracketCount > 0) {
                    g_lexerState.bracketCount--;
                }
                g_lexerState.lineHasCode = true;
                return 0;
            }
            case ';': {
                token->type = TOKEN_SEMICOLON;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            case ':': {
                token->type = TOKEN_COLON;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            case '.': {
                currState = STATE_PERIOD;
                break;
            }
            case ',': {
                token->type = TOKEN_COMMA;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            case '+': {
                token->type = TOKEN_PLUS;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            case '-': {
                token->type = TOKEN_MINUS;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            case '*': {
                token->type = TOKEN_MULTIPLY;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            case '/': {
                currState = STATE_SLASH;
                break;
            }
            case '<': {
                currState = STATE_ANGLE_BRACKET_LESSER;
                break;
            }
            case '>': {
                currState = STATE_ANGLE_BRACKET_BIGGER;
                break;
            }
            case '=': {
                currState = STATE_EQUAL;
                break;
            }
            case '!': {
                currState = STATE_EXCLAMATION_MARK;
                break;
            }
            case '@': {
                if (strInit(&dynStr) != 0) {
                    return internalError("memory allocation failed");
                }
                currState = STATE_AT_SIGN;
                break;
            }
            case '"': {
                if (strInit(&dynStr) != 0) {
                    return internalError("memory allocation failed");
                }
                currState = STATE_STRING_START;
                break;
            }

            case '0': {
                if (strInit(&dynStr) != 0) {
                    return internalError("memory allocation failed");
                }
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_ZERO_NUMBER;
                break;
            }
            default: {
                return lexicalError("unexpected character"); // unknown char
            }
            }
            break;
        }
        case STATE_IDENTIFIER: // identifier or keyword
        {
            if (isalnum(currChar) || currChar == '_') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                break;
            }
            (void)ungetc(currChar, stdin);
            if (strCompare(&dynStr, "_") == 0) {
                return lexicalErrorFree(&dynStr, "standalone '_' is not a valid identifier");
            }
            keyword keywordVal;
            if (checkForKeyword(&dynStr, &keywordVal)) {
                token->type = TOKEN_KEYWORD;
                token->data.keywordVal = keywordVal;
                g_lexerState.lineHasCode = true;
                strFree(&dynStr);
            } else {
                token->type = TOKEN_IDENTIFIER;
                g_lexerState.lineHasCode = true;
                token->data.identifier = malloc(strlen(dynStr.str) + 1);
                if (token->data.identifier == NULL) {
                    return internalErrorFree(&dynStr, "memory allocation failed for identifier");
                }
                strcpy(token->data.identifier, dynStr.str);
                strFree(&dynStr);
            }
            token->line = g_lexerState.currLine;
            return 0; // valid token
        }
        case STATE_ZERO_NUMBER: { // number starting with 0
            if (currChar == '.') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_DECIMAL_POINT;
            } else if (currChar == 'e' || currChar == 'E') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_E;
            } else if (currChar == 'd' || currChar == 'D') {
                currState = STATE_DOUBLE_SUFFIX;
            } else if (isdigit(currChar)) { // lex error, whole number cant start with zero
                return lexicalErrorFree(&dynStr, "integer literal cannot start with leading zero");
            } else {
                (void)ungetc(currChar, stdin);
                token->type = TOKEN_INT;
                token->line = g_lexerState.currLine;
                token->data.intVal = (int)strtol(dynStr.str, NULL, 10);
                g_lexerState.lineHasCode = true;
                strFree(&dynStr);
                return 0;
            }
            break;
        }
        case STATE_FULL_NUMBER: {
            if (currChar == '_') { // underscore is not included in the final number
                currState = STATE_FULL_NUMBER_UNDERSCORE;

            } else if (currChar >= '0' && currChar <= '9') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_FULL_NUMBER; // stays in the same state

            } else if (currChar == 'e' || currChar == 'E') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_E;

            } else if (currChar == 'd' || currChar == 'D') {
                currState = STATE_DOUBLE_SUFFIX;

            } else if (currChar == '.') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_DECIMAL_POINT;

            } else {
                (void)ungetc(currChar, stdin);
                token->type = TOKEN_INT;
                token->line = g_lexerState.currLine;
                token->data.intVal = (int)strtol(dynStr.str, NULL, 10);
                g_lexerState.lineHasCode = true;
                strFree(&dynStr);
                return 0;
            }

            break;
        }
        case STATE_FULL_NUMBER_UNDERSCORE: {
            if (currChar == '_') {
                currState = STATE_FULL_NUMBER_UNDERSCORE;
            } else if (currChar >= '0' && currChar <= '9') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_FULL_NUMBER;
            } else {
                return lexicalErrorFree(
                    &dynStr, "expected digit after underscore in number literal");
            }
            break;
        }

        case STATE_DECIMAL_POINT: // expecting a decimal number
        {
            if (currChar >= '0' && currChar <= '9') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_DECIMAL_NUMBER;

            } else {
                return lexicalErrorFree(&dynStr, "expected digit after decimal point");
            }
            break;
        }
        case STATE_DECIMAL_NUMBER: {
            if (currChar == '_') {
                currState = STATE_DECIMAL_NUMBER_UNDERSCORE;
            } else if (currChar >= '0' && currChar <= '9') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_DECIMAL_NUMBER;
            } else if (currChar == 'e' || currChar == 'E') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_E;

            } else if (currChar == 'd' || currChar == 'D') {
                currState = STATE_DOUBLE_SUFFIX;

            } else {
                (void)ungetc(currChar, stdin);
                token->type = TOKEN_DOUBLE;
                token->line = g_lexerState.currLine;
                token->data.doubleVal = strtod(dynStr.str, NULL);
                g_lexerState.lineHasCode = true;
                strFree(&dynStr);
                return 0;
            }
            break;
        }
        case STATE_DECIMAL_NUMBER_UNDERSCORE: { // underscore state inside decimal number
            if (currChar == '_') {
                currState = STATE_DECIMAL_NUMBER_UNDERSCORE;

            } else if (currChar >= '0' && currChar <= '9') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_DECIMAL_NUMBER;
            } else {
                return lexicalErrorFree(
                    &dynStr, "expected digit after underscore in decimal literal");
            }
            break;
        }
        case STATE_EXPONENT_E: {
            if (currChar == '-' || currChar == '+') { // expecting optional sign
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_SIGN;

            } else if (currChar >= '0' && currChar <= '9') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_NUMBER;
            } else {
                return lexicalErrorFree(&dynStr, "expected sign or digit in exponent");
            }
            break;
        }
        case STATE_EXPONENT_SIGN: {
            if (currChar >= '0' && currChar <= '9') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_NUMBER;
            } else {
                return lexicalErrorFree(&dynStr, "expected digit after exponent sign");
            }
            break;
        }
        case STATE_EXPONENT_NUMBER: {
            if (currChar == '_') {
                currState = STATE_EXPONENT_NUMBER_UNDERSCORE;
            } else if (currChar >= '0' && currChar <= '9') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_NUMBER;
            } else if (currChar == 'd' || currChar == 'D') {
                currState = STATE_DOUBLE_SUFFIX;

            } else {
                (void)ungetc(currChar, stdin);
                token->type = TOKEN_DOUBLE;
                token->line = g_lexerState.currLine;
                token->data.doubleVal = strtod(dynStr.str, NULL);
                g_lexerState.lineHasCode = true;
                strFree(&dynStr);
                return 0;
            }
            break;
        }
        case STATE_EXPONENT_NUMBER_UNDERSCORE: {
            if (currChar == '_') {
                currState = STATE_EXPONENT_NUMBER_UNDERSCORE;

            } else if (currChar >= '0' && currChar <= '9') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_NUMBER;
            } else {
                return lexicalErrorFree(&dynStr, "expected digit after underscore in exponent");
            }
            break;
        }
        case STATE_DOUBLE_SUFFIX: { // d or D after number
            (void)ungetc(currChar, stdin);
            token->type = TOKEN_DOUBLE;
            token->line = g_lexerState.currLine;
            token->data.doubleVal = strtod(dynStr.str, NULL);
            g_lexerState.lineHasCode = true;
            strFree(&dynStr);
            return 0;
        }
        case STATE_SLASH: { // recognize comments or divide
            if (currChar == '/') {
                currState = STATE_LINE_COMMENT;
            } // line comment
            else if (currChar == '*') {
                currState = STATE_BLOCK_COMMENT;
                commentDepth = 1;
                commentHasNewline = false;
            } else {
                (void)ungetc(currChar, stdin);
                token->type = TOKEN_DIVIDE;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            break;
        }
        case STATE_LINE_COMMENT: {
            if (currChar == '\n') {
                spaceCount = 0;
                if (g_lexerState.bracketCount > 0) { // inside brackets
                    g_lexerState.isNewLine = false;
                    g_lexerState.currLine++;
                    currState = STATE_START;
                    break;
                }
                g_lexerState.isNewLine = true; // outside brackets
                if (g_lexerState.lineHasCode) {
                    token->type = TOKEN_EOL;
                    token->line = g_lexerState.currLine;
                    g_lexerState.lineHasCode = false;
                    g_lexerState.currLine++;
                    return 0;
                }
                g_lexerState.currLine++; // comment only line, skip with no eol emit
                currState = STATE_START;
                break;
            }
            if (currChar == EOF) {
                if (g_lexerState.bracketCount == 0 && g_lexerState.lineHasCode) {
                    token->type = TOKEN_EOL;
                    token->line = g_lexerState.currLine;
                    g_lexerState.lineHasCode = false;
                    g_lexerState.isNewLine = true;
                    (void)ungetc(currChar, stdin); // push eof back for dedicated handling
                    return 0;
                }
                (void)ungetc(currChar, stdin);
                spaceCount = 0;
                currState = STATE_PROCESS_INDENT;
                break;
            }
            currState = STATE_LINE_COMMENT;
            break;
        }
        case STATE_ANGLE_BRACKET_LESSER: { // < or <=
            if (currChar == '=') {
                token->type = TOKEN_COMPARE_LESSER_EQUAL;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            (void)ungetc(currChar, stdin);
            token->type = TOKEN_COMPARE_SMALLER_THAN;
            token->line = g_lexerState.currLine;
            g_lexerState.lineHasCode = true;
            return 0;
        }
        case STATE_ANGLE_BRACKET_BIGGER: { // > or >=
            if (currChar == '=') {
                token->type = TOKEN_COMPARE_BIGGER_EQUAL;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            (void)ungetc(currChar, stdin);
            token->type = TOKEN_COMPARE_BIGGER_THAN;
            token->line = g_lexerState.currLine;
            g_lexerState.lineHasCode = true;
            return 0;
        }
        case STATE_EQUAL: { // = or ==

            if (currChar == '=') {
                token->type = TOKEN_EQUAL_TO;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            (void)ungetc(currChar, stdin);
            token->type = TOKEN_EQUAL_SIGN;
            token->line = g_lexerState.currLine;
            g_lexerState.lineHasCode = true;
            return 0;
        }
        case STATE_EXCLAMATION_MARK: { // only != is supported
            if (currChar == '=') {
                token->type = TOKEN_NOT_EQUAL_TO;
                g_lexerState.lineHasCode = true;
                token->line = g_lexerState.currLine;
                return 0;
            }
            (void)ungetc(currChar, stdin);

            return lexicalError("unexpected '!', expected '!='");
        }
        case STATE_LINE_BEG: { // count spaces in beggining of line
            if (currChar == ' ') {
                spaceCount++;
                break;
            }
            if (currChar == '\t') {
                return lexicalError("tab character is not allowed for indentation");
            }
            if (currChar == '\n') // empty line
            {
                g_lexerState.currLine++;
                g_lexerState.isNewLine = true;
                spaceCount = 0;
                currState = STATE_START;
                break;
            }
            if (currChar == '/') {
                currState = STATE_LINE_BEG_WITH_SLASH; // need to check what the next char is
                break;
            }
            if (currChar == EOF) {
                (void)ungetc(currChar, stdin);
                spaceCount = 0; // reset indent to zero
                currState = STATE_PROCESS_INDENT;
                break;
            }
            // valid char found, need to process indent level
            currState = STATE_PROCESS_INDENT;
            (void)ungetc(currChar, stdin);
            break;
        }
        case STATE_LINE_BEG_WITH_SLASH: { // / encountered on empty line
            if (currChar == '/') {
                currState = STATE_LINE_COMMENT;
                break;
            }
            if (currChar == '*') {
                currState = STATE_BLOCK_COMMENT;
                commentDepth = 1;
                commentHasNewline = false;
                break;
            }
            // standalone divide, gets emitted later
            g_lexerState.pendingDivide = true;
            (void)ungetc(currChar, stdin);
            currState = STATE_PROCESS_INDENT;
            break;
        }
        case STATE_BLOCK_COMMENT: // inside block comment
        {
            if (currChar == '\n') {
                commentHasNewline = true;
                g_lexerState.currLine++;
                break;
            }
            if (currChar == '/') {
                currState = STATE_BLOCK_COMMENT_SLASH;
                break;
            }
            if (currChar == '*') {
                currState = STATE_BLOCK_COMMENT_STAR;
                break;
            }
            if (currChar == EOF) // ERROR
            {
                return lexicalError("unterminated block comment");
            }
            break; // other chars are ignored
        }
        case STATE_BLOCK_COMMENT_SLASH: { // check for nested block comment
            if (currChar == '*') {        // nested block comment
                commentDepth++;
                currState = STATE_BLOCK_COMMENT;
                break;
            }
            if (currChar == '\n') {
                commentHasNewline = true;
                g_lexerState.currLine++;
                currState = STATE_BLOCK_COMMENT;
                break;
            }
            if (currChar == EOF) // ERROR
            {
                return lexicalError("unterminated block comment");
            }
            if (currChar == '/') {
                currState = STATE_BLOCK_COMMENT_SLASH;
                break;
            }
            currState = STATE_BLOCK_COMMENT;
            break;
        }
        case STATE_BLOCK_COMMENT_STAR: { // check for comment end
            if (currChar == '/') {
                commentDepth--;
                if (commentDepth > 0) {
                    currState = STATE_BLOCK_COMMENT;
                    break;
                }
                if (commentHasNewline) {
                    currState = STATE_AFTER_MULTILINE_BLOCK_COMMENT;
                } else if (g_lexerState.lineHasCode) {
                    currState = STATE_START;
                } else {
                    currState = STATE_LINE_BEG_AFTER_BLOCK_COMMENT;
                }
                break;
            }
            if (currChar == '*') {
                // stay in state
                break;
            }
            if (currChar == EOF) // ERROR
            {
                return lexicalError("unterminated block comment");
            }
            if (currChar == '\n') {
                commentHasNewline = true;
                g_lexerState.currLine++;
                currState = STATE_BLOCK_COMMENT;
                break;
            }
            currState = STATE_BLOCK_COMMENT;
            break;
        }
        case STATE_LINE_BEG_AFTER_BLOCK_COMMENT: // line beg, processed block comment
        {
            if (currChar == ' ' || currChar == '\t') {
                // ignore
                break;
            }
            if (currChar == '/') {
                currState = STATE_LINE_BEG_WITH_SLASH;
                break;
            }
            if (currChar == '\n') // empty line, ignore indent
            {
                spaceCount = 0;
                currState = STATE_LINE_BEG;
                g_lexerState.isNewLine = true;
                g_lexerState.currLine++;
                break;
            }
            if (currChar == EOF) {
                currState = STATE_START;
                spaceCount = 0;
                (void)ungetc(currChar, stdin);
                break;
            }
            // normal code, process indent
            (void)ungetc(currChar, stdin);
            currState = STATE_PROCESS_INDENT;
            break;
        }
        case STATE_PROCESS_INDENT: {
            // calculates needed indent/ dedent tokens
            int topIndent = g_lexerState.indentStack[g_lexerState.stackCount - 1];
            if (currChar == EOF && topIndent == 0 && !g_lexerState.pendingDivide) { // process eof
                token->type = TOKEN_EOF;
                token->line = g_lexerState.currLine;
                return 0;
            }
            if (spaceCount > topIndent) { // indent occured, emit indent
                if (g_lexerState.stackCount >= MAX_INDENT_SIZE) {
                    return internalError("maximum indentation nesting depth exceeded");
                }
                g_lexerState.indentStack[g_lexerState.stackCount] = spaceCount;
                g_lexerState.stackCount++;
                g_lexerState.isNewLine = false;
                token->type = TOKEN_INDENT;
                token->line = g_lexerState.currLine;
                (void)ungetc(currChar, stdin);
                return 0;
            } else if (spaceCount < topIndent) { // dedent occured, emit dedent
                int dedentsToEmit = 0;
                while (g_lexerState.stackCount > 0 &&
                       g_lexerState.indentStack[g_lexerState.stackCount - 1] > spaceCount) {
                    g_lexerState.stackCount--;
                    dedentsToEmit++; // multiple dedents must be emited
                }
                if (g_lexerState.indentStack[g_lexerState.stackCount - 1] != spaceCount) {
                    return lexicalError("inconsistent dedentation level");
                }
                g_lexerState.pendingDedents = dedentsToEmit - 1;
                if (currChar != EOF) {
                    g_lexerState.isNewLine = false;
                }
                token->type = TOKEN_DEDENT;
                token->line = g_lexerState.currLine;
                (void)ungetc(currChar, stdin);
                return 0;
            } else { // not indent nor dedent, continue from start
                g_lexerState.isNewLine = false;
                currState = STATE_START;
                (void)ungetc(currChar, stdin);
                break;
            }
        }
        case STATE_AFTER_MULTILINE_BLOCK_COMMENT: // if block comment on multiple lines, no code can
                                                  // follow
        {
            if (currChar == ' ' || currChar == '\t') {
                break; // whitespace is okey
            }
            if (currChar == '\n') {
                if (g_lexerState.bracketCount > 0) {
                    g_lexerState.isNewLine = false;
                    spaceCount = 0;
                    g_lexerState.currLine++;
                    currState = STATE_START;
                    break;
                }
                if (g_lexerState.lineHasCode) {
                    spaceCount = 0;
                    g_lexerState.lineHasCode = false;
                    g_lexerState.isNewLine = true;
                    currState = STATE_START;
                    token->type = TOKEN_EOL;
                    token->line = g_lexerState.currLine;
                    g_lexerState.currLine++;

                    return 0;
                } else { // comment on empty line is ignored, no eol emited
                    g_lexerState.lineHasCode = false;
                    g_lexerState.isNewLine = true;
                    spaceCount = 0;
                    g_lexerState.currLine++;
                    currState = STATE_START;
                    break;
                }
            }
            if (currChar == EOF) {
                spaceCount = 0;
                (void)ungetc(currChar, stdin);
                if (g_lexerState.bracketCount == 0 && g_lexerState.lineHasCode) {
                    g_lexerState.lineHasCode = false;
                    g_lexerState.isNewLine = true;
                    token->type = TOKEN_EOL;
                    token->line = g_lexerState.currLine;
                    return 0;
                }
                currState = STATE_PROCESS_INDENT;
                break;
            }
            // any other char, error
            return lexicalError("unexpected token after multiline block comment");
        }
        case STATE_AT_SIGN: { // @ processing
            if (isalnum(currChar) || currChar == '_') {
                if (strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                break;
            }
            (void)ungetc(currChar, stdin);
            if (strCompare(&dynStr, "main") == 0) {
                strFree(&dynStr);
                token->type = TOKEN_MAIN;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            return lexicalErrorFree(&dynStr, "invalid annotation, only '@main' is allowed");
        }
        case STATE_STRING_START: { // " seen, need to determine if single or multiline string
            if (currChar == '"') {
                currState = STATE_STRING_SECOND_QUOTE;
                break;
            }
            if (currChar == '\\') {
                currState = STATE_STRING_ESCAPE_SEQUENCE;
                break;
            }
            if (currChar == '\n' || currChar == '\r' || currChar == EOF) {
                return lexicalErrorFree(&dynStr, "unterminated string literal");
            }
            if (strAddChar(&dynStr, (char)currChar) != 0) {
                return internalErrorFree(&dynStr, "memory allocation failed");
            }
            currState = STATE_STRING;
            break;
        }
        case STATE_STRING_SECOND_QUOTE: { // ""
            if (currChar == '"') {
                currState = STATE_MULTILINE_STRING;
                break;
            }
            // empty string
            (void)ungetc(currChar, stdin);
            return finishStringToken(token, &dynStr);
        }
        case STATE_STRING: {
            if (currChar == '"') { // end of string
                return finishStringToken(token, &dynStr);
            }
            if (currChar == '\\') {
                currState = STATE_STRING_ESCAPE_SEQUENCE; // escape sequence
                break;
            }
            if (currChar == '\n' || currChar == '\r' ||
                currChar == EOF) { // not allowed on single line string
                return lexicalErrorFree(&dynStr, "unterminated string literal");
            }
            if (strAddChar(&dynStr, (char)currChar) != 0) {
                return internalErrorFree(&dynStr, "memory allocation failed");
            }
            break;
        }
        case STATE_STRING_ESCAPE_SEQUENCE: { // escape sequence
            char escapedChar;
            switch (currChar) { // these are allowed:  \\, \", \n, \r a \t.
            case '\\':
                escapedChar = '\\';
                break;
            case '"':
                escapedChar = '"';
                break;
            case 'n':
                escapedChar = '\n';
                break;
            case 'r':
                escapedChar = '\r';
                break;
            case 't':
                escapedChar = '\t';
                break;
            default: // unsupported char
                return lexicalErrorFree(&dynStr, "invalid escape sequence in string literal");
            }
            if (strAddChar(&dynStr, escapedChar) != 0) {
                return internalErrorFree(&dynStr, "memory allocation failed");
            }
            currState = STATE_STRING;
            break;
        }
        case STATE_MULTILINE_STRING: { // body of multiline string
            if (currChar == EOF) {     // unterminated string
                return lexicalErrorFree(&dynStr, "unterminated multiline string literal");
            }
            if (currChar == '"') {
                currState = STATE_MULTILINE_QUOTE1;
                break;
            }
            if (currChar == '\n') {
                g_lexerState.currLine++;
            }
            if (strAddChar(&dynStr, (char)currChar) != 0) {
                return internalErrorFree(&dynStr, "memory allocation failed");
            }
            break;
        }

        case STATE_MULTILINE_QUOTE1: { // """ "
            if (currChar == EOF) {
                return lexicalErrorFree(&dynStr, "unterminated multiline string literal");
            }
            if (currChar == '"') {
                currState = STATE_MULTILINE_QUOTE2;
                break;
            }
            if (currChar == '\n') {
                g_lexerState.currLine++;
            }
            if (strAddChar(&dynStr, '"') != 0 ||
                strAddChar(&dynStr, (char)currChar) != 0) { // we add " from previous read
                return internalErrorFree(&dynStr, "memory allocation failed");
            }
            currState = STATE_MULTILINE_STRING;
            break;
        }
        case STATE_MULTILINE_QUOTE2: {
            if (currChar == EOF) { // error
                return lexicalErrorFree(&dynStr, "unterminated multiline string literal");
            }
            if (currChar == '"') {
                currState = STATE_MULTILINE_END; // three " in a row
                break;
            }
            if (currChar == '\n') {
                g_lexerState.currLine++;
            }
            if (strAddChar(&dynStr, '"') != 0 ||
                strAddChar(&dynStr, '"') != 0 || // not end yet, return "" to the string
                strAddChar(&dynStr, (char)currChar) != 0) {
                return internalErrorFree(&dynStr, "memory allocation failed");
            }
            currState = STATE_MULTILINE_STRING;
            break;
        }
        case STATE_MULTILINE_END: { // extra " are added to the string, otherwise finish it

            if (currChar == '"') {
                if (strAddChar(&dynStr, '"') != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                break;
            }
            (void)ungetc(currChar, stdin);
            return finishStringToken(token, &dynStr);
        }
        case STATE_PERIOD: { // check if just dot or .x number
            if (currChar >= '0' && currChar <= '9') {
                if (strInit(&dynStr) != 0) {
                    return internalError("memory allocation failed");
                }
                if (strAddChar(&dynStr, '.') != 0 || strAddChar(&dynStr, (char)currChar) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_DECIMAL_NUMBER;
                break;
            }
            (void)ungetc(currChar, stdin);
            token->type = TOKEN_PERIOD;
            token->line = g_lexerState.currLine;
            g_lexerState.lineHasCode = true;
            return 0;
        }
        default: {
            break;
        }
        }
    }
}
