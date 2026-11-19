/**
 * @file    lexer.c
 * @author  Jakub Zenata (xzenatj00)
 * @brief   lexer implementation
 * @date    2026-11-19
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
    int indentStack[MAX_INDENT_SIZE];
    int stackCount;
    bool isNewLine;
    int pendingDedents; // in case multiple indent blocks end at the same time
    // and lexer has to return multiple dedent tokens in a row
    int bracketCount; // needed for eol in brackets handling
    bool pendingDivide;
    bool lineHasCode; // potreba pro vyreseni zda vratit eol
} lexerState;
typedef struct TKeywordPair {
    const char *str;
    keyword keywordVal;
} KeywordPair; // struct used to compare string with keywords

static lexerState g_lexerState; // private lexer state,
// used primarelly for parsing indent/dedent blocks
static bool g_lexerStateInitialized;

static int lexicalError(const char *msg) {
    errorLexer(g_lexerState.currLine, msg);
    return ERR_LEXER;
}
static int lexicalErrorFree(DynamicString *dynStr, const char *msg) {
    strFree(dynStr);
    errorLexer(g_lexerState.currLine, msg);
    return ERR_LEXER;
}
static int internalError(const char *msg) {
    errorInternal(g_lexerState.currLine, msg);
    return ERR_INTERNAL;
}
static int internalErrorFree(DynamicString *dynStr, const char *msg) {
    strFree(dynStr);
    errorInternal(g_lexerState.currLine, msg);
    return ERR_INTERNAL;
}
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
static bool checkForKeyword(DynamicString *str1, keyword *detectedKeyword) {
    static const KeywordPair keywords[] = {
        {"def", KEYWORD_DEF},   {"do", KEYWORD_DO},         {"Double", KEYWORD_DOUBLE},
        {"else", KEYWORD_ELSE}, {"if", KEYWORD_IF},         {"import", KEYWORD_IMPORT},
        {"Int", KEYWORD_INT},   {"return", KEYWORD_RETURN}, {"String", KEYWORD_STRING},
        {"then", KEYWORD_THEN}, {"Unit", KEYWORD_UNIT},     {"val", KEYWORD_VAL},
        {"var", KEYWORD_VAR},   {"while", KEYWORD_WHILE},   {"Boolean", KEYWORD_BOOLEAN},
        {"case", KEYWORD_CASE}, {"class", KEYWORD_CLASS},   {"extension", KEYWORD_EXTENSION},
        {"for", KEYWORD_FOR},   {"List", KEYWORD_LIST},     {"match", KEYWORD_MATCH},
        {"type", KEYWORD_TYPE}};
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strCompare(str1, keywords[i].str) == 0) {
            *detectedKeyword = keywords[i].keywordVal;
            return true;
        }
    }
    return false;
}

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
int getToken(Token *token) {
    if (!g_lexerStateInitialized) {
        errorInternal(
            0, "lexerStateInit was not called before getToken"); // nemuzu volat internalError()
                                                                 // protoze accesuje
                                                                 // neinicializovany lexerstate
        return ERR_INTERNAL;
    }
    int currState = STATE_START;
    DynamicString dynStr;
    int spaceCount = 0;
    int commentDepth = 0;           // pro vnorene blokove komentare
    bool commentHasNewline = false; // pro blokove komentare
    while (true) {
        int znak = getchar();
        switch (currState) {
        case STATE_START: {                      // pocatecni stav
            if (g_lexerState.pendingDedents > 0) // vracime dedenty ktere dluzime
            {
                (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy;
                g_lexerState.pendingDedents--;
                token->type = TOKEN_DEDENT;
                token->line = g_lexerState.currLine;
                return 0;
            }
            if (g_lexerState.pendingDivide) {
                (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy;
                g_lexerState.pendingDivide = false;
                token->type = TOKEN_DIVIDE;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            if (g_lexerState.isNewLine) {
                (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy;
                currState = STATE_LINE_BEG;
                break;
            }
            if (znak == EOF) {
                spaceCount = 0;
                (void)ungetc(znak, stdin);
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
            if (znak == '\n') {
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
                break; // newline je uvnitr zavorek, pokracujeme ve cteni
            }
            if (znak == ' ' || znak == '\t') {
                break;
            }
            if (znak == '(') {
                token->type = TOKEN_LEFT_BRACKET;
                token->line = g_lexerState.currLine;
                g_lexerState.bracketCount++;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            if (znak == ')') {
                token->type = TOKEN_RIGHT_BRACKET;
                token->line = g_lexerState.currLine;
                if (g_lexerState.bracketCount > 0) {
                    g_lexerState.bracketCount--;
                }
                g_lexerState.lineHasCode = true;
                return 0;
            }
            if (znak == ';') {
                token->type = TOKEN_SEMICOLON;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            if (znak == ':') {
                token->type = TOKEN_COLON;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            if (znak == '.') {
                token->type = TOKEN_PERIOD;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            if (znak == ',') {
                token->type = TOKEN_COMMA;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            if (znak == '+') {
                token->type = TOKEN_PLUS;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            if (znak == '-') {
                token->type = TOKEN_MINUS;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            if (znak == '*') {
                token->type = TOKEN_MULTIPLY;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            if (znak == '/') {
                currState = STATE_SLASH;
                break;
            }
            if (znak == '<') {
                currState = STATE_ANGLE_BRACKET_LESSER;
                break;
            }
            if (znak == '>') {
                currState = STATE_ANGLE_BRACKET_BIGGER;
                break;
            }
            if (znak == '=') {
                currState = STATE_EQUAL;
                break;
            }
            if (znak == '!') {
                currState = STATE_EXCLAMATION_MARK;
                break;
            }
            if (znak == '@') {
                if (strInit(&dynStr) != 0) {
                    return internalError("memory allocation failed");
                }
                currState = STATE_AT_SIGN;
                break;
            }
            if (znak == '"') {
                if (strInit(&dynStr) != 0) {
                    return internalError("memory allocation failed");
                }
                currState = STATE_STRING_START;
                break;
            }
            if (isalpha(znak) || znak == '_') {
                if (strInit(&dynStr) != 0) {
                    return internalError("memory allocation failed");
                }
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_IDENTIFIER; // identifikator
                break;
            }
            if (znak == '0') {
                if (strInit(&dynStr) != 0) {
                    return internalError("memory allocation failed");
                }
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_ZERO_NUMBER;
                break;
            }
            if (znak > '0' && znak <= '9') {
                if (strInit(&dynStr) != 0) {
                    return internalError("memory allocation failed");
                }
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_FULL_NUMBER; // cislo
                break;
            }
            return lexicalError("unexpected character"); // nezname znaky
        }
        case STATE_IDENTIFIER: // identifikator
        {
            if (isalnum(znak) || znak == '_') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                break;
            }
            (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy;
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
            return 0; // validni token, konec cele funkce
        }
        case STATE_ZERO_NUMBER: {
            if (znak == '.') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_DECIMAL_POINT;
            } else if (znak == 'e' || znak == 'E') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_E;
            } else if (znak == 'd' || znak == 'D') {
                currState = STATE_DOUBLE_SUFFIX;
            } else if (isdigit(znak)) { // lex chyba, cele cislo nesmi zacinat nulou
                return lexicalErrorFree(&dynStr, "integer literal cannot start with leading zero");
            } else {
                (void)ungetc(znak, stdin);
                token->type = TOKEN_INT;
                token->line = g_lexerState.currLine;
                char *endptr;
                token->data.intVal = (int)strtol(dynStr.str, &endptr, 10);
                g_lexerState.lineHasCode = true;
                strFree(&dynStr);
                return 0;
            }
            break;
        }
        case STATE_FULL_NUMBER: // cislo
        {
            if (znak == '_') {
                currState = STATE_FULL_NUMBER_UNDERSCORE;

            } else if (znak >= '0' && znak <= '9') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_FULL_NUMBER; // zustavame ve stejnem stavu

            } else if (znak == 'e' || znak == 'E') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_E;

            } else if (znak == 'd' || znak == 'D') {
                currState = STATE_DOUBLE_SUFFIX;

            } else if (znak == '.') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_DECIMAL_POINT;

            } else {
                (void)ungetc(znak, stdin);
                token->type = TOKEN_INT;
                char *endptr;
                token->line = g_lexerState.currLine;
                token->data.intVal = (int)strtol(dynStr.str, &endptr, 10);
                g_lexerState.lineHasCode = true;
                strFree(&dynStr);
                return 0;
            }

            break;
        }
        case STATE_FULL_NUMBER_UNDERSCORE: {
            if (znak == '_') {
                currState = STATE_FULL_NUMBER_UNDERSCORE;
            } else if (znak >= '0' && znak <= '9') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_FULL_NUMBER;
            } else {
                return lexicalErrorFree(&dynStr,
                                        "expected digit after underscore in number literal");
            }
            break;
        }

        case STATE_DECIMAL_POINT: // cislo
        {
            if (znak >= '0' && znak <= '9') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_DECIMAL_NUMBER;

            } else {
                return lexicalErrorFree(&dynStr, "expected digit after decimal point");
            }
            break;
        }
        case STATE_DECIMAL_NUMBER: {
            if (znak == '_') {
                currState = STATE_DECIMAL_NUMBER_UNDERSCORE;
            } else if (znak >= '0' && znak <= '9') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_DECIMAL_NUMBER;
            } else if (znak == 'e' || znak == 'E') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_E;

            } else if (znak == 'd' || znak == 'D') {
                currState = STATE_DOUBLE_SUFFIX;

            } else {
                (void)ungetc(znak, stdin);
                token->type = TOKEN_DOUBLE;
                char *endptr;
                token->line = g_lexerState.currLine;
                token->data.doubleVal = strtod(dynStr.str, &endptr);
                g_lexerState.lineHasCode = true;
                strFree(&dynStr);
                return 0;
            }
            break;
        }
        case STATE_DECIMAL_NUMBER_UNDERSCORE: {
            if (znak == '_') {
                currState = STATE_DECIMAL_NUMBER_UNDERSCORE;

            } else if (znak >= '0' && znak <= '9') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_DECIMAL_NUMBER;
            } else {
                return lexicalErrorFree(&dynStr,
                                        "expected digit after underscore in decimal literal");
            }
            break;
        }
        case STATE_EXPONENT_E: {
            if (znak == '-' || znak == '+') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_SIGN;

            } else if (znak >= '0' && znak <= '9') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_NUMBER;
            } else {
                return lexicalErrorFree(&dynStr, "expected sign or digit in exponent");
            }
            break;
        }
        case STATE_EXPONENT_SIGN: {
            if (znak >= '0' && znak <= '9') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_NUMBER;
            } else {
                return lexicalErrorFree(&dynStr, "expected digit after exponent sign");
            }
            break;
        }
        case STATE_EXPONENT_NUMBER: {
            if (znak == '_') {
                currState = STATE_EXPONENT_NUMBER_UNDERSCORE;
            } else if (znak >= '0' && znak <= '9') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_NUMBER;
            } else if (znak == 'd' || znak == 'D') {
                currState = STATE_DOUBLE_SUFFIX;

            } else {
                (void)ungetc(znak, stdin);
                token->type = TOKEN_DOUBLE;
                char *endptr;
                token->line = g_lexerState.currLine;
                token->data.doubleVal = strtod(dynStr.str, &endptr);
                g_lexerState.lineHasCode = true;
                strFree(&dynStr);
                return 0;
            }
            break;
        }
        case STATE_EXPONENT_NUMBER_UNDERSCORE: {
            if (znak == '_') {
                currState = STATE_EXPONENT_NUMBER_UNDERSCORE;

            } else if (znak >= '0' && znak <= '9') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                currState = STATE_EXPONENT_NUMBER;
            } else {
                return lexicalErrorFree(&dynStr, "expected digit after underscore in exponent");
            }
            break;
        }
        case STATE_DOUBLE_SUFFIX: {
            (void)ungetc(znak, stdin);
            token->type = TOKEN_DOUBLE;
            char *endptr;
            token->line = g_lexerState.currLine;
            token->data.doubleVal = strtod(dynStr.str, &endptr);
            g_lexerState.lineHasCode = true;
            strFree(&dynStr);
            return 0;
        }
        case STATE_SLASH: {
            if (znak == '/') {
                currState = STATE_LINE_COMMENT;
            } // stav pro radkovy komentar
            else if (znak == '*') {
                currState = STATE_BLOCK_COMMENT;
                commentDepth = 1;
                commentHasNewline = false;
            } else {
                (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy
                token->type = TOKEN_DIVIDE;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            break;
        }
        case STATE_LINE_COMMENT: // radkovy komentar
        {
            if (znak == '\n') {
                spaceCount = 0;
                if (g_lexerState.bracketCount > 0) { // jsme v zavorkach
                    g_lexerState.isNewLine = false;
                    g_lexerState.currLine++;
                    currState = STATE_START;
                    break;
                }
                g_lexerState.isNewLine = true; // nejsme v zavorkach
                if (g_lexerState.lineHasCode) {
                    token->type = TOKEN_EOL;
                    token->line = g_lexerState.currLine;
                    g_lexerState.lineHasCode = false;
                    g_lexerState.currLine++;
                    return 0;
                }
                g_lexerState.currLine++; // an radku je jen komentar
                currState = STATE_START;
                break;
            }
            if (znak == EOF) {
                if (g_lexerState.bracketCount == 0 && g_lexerState.lineHasCode) {
                    token->type = TOKEN_EOL;
                    token->line = g_lexerState.currLine;
                    g_lexerState.lineHasCode = false;
                    g_lexerState.isNewLine = true;
                    (void)ungetc(znak, stdin); // vratime eof pro samostatne zpracovani
                    return 0;
                }
                (void)ungetc(znak, stdin); // vratime eof pro samostatne zpracovani
                spaceCount = 0;
                currState = STATE_PROCESS_INDENT;
                break;
            }
            currState = STATE_LINE_COMMENT;
            break;
        }
        case STATE_ANGLE_BRACKET_LESSER: {
            if (znak == '=') {
                token->type = TOKEN_COMPARE_LESSER_EQUAL;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy
            token->type = TOKEN_COMPARE_SMALLER_THAN;
            token->line = g_lexerState.currLine;
            g_lexerState.lineHasCode = true;
            return 0;
        }
        case STATE_ANGLE_BRACKET_BIGGER: {
            if (znak == '=') {
                token->type = TOKEN_COMPARE_BIGGER_EQUAL;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy
            token->type = TOKEN_COMPARE_BIGGER_THAN;
            token->line = g_lexerState.currLine;
            g_lexerState.lineHasCode = true;
            return 0;
        }
        case STATE_EQUAL: {

            if (znak == '=') {
                token->type = TOKEN_EQUAL_TO;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy
            token->type = TOKEN_EQUAL_SIGN;
            token->line = g_lexerState.currLine;
            g_lexerState.lineHasCode = true;
            return 0;
        }
        case STATE_EXCLAMATION_MARK: {
            if (znak == '=') {
                token->type = TOKEN_NOT_EQUAL_TO;
                g_lexerState.lineHasCode = true;
                token->line = g_lexerState.currLine;
                return 0;
            }
            (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy

            return lexicalError("unexpected '!', expected '!='");
        }
        case STATE_LINE_BEG: {
            if (znak == ' ') {
                spaceCount++;
                break;
            }
            if (znak == '\t') // jedna se o prazdny radek
            {
                return lexicalError("tab character is not allowed for indentation");
            }
            if (znak == '\n') // jedna se o prazdny radek
            {
                g_lexerState.currLine++;
                g_lexerState.isNewLine = true;
                spaceCount = 0;
                currState = STATE_START;
                break;
            }
            if (znak == '/') {
                currState = STATE_LINE_BEG_WITH_SLASH;
                break;
            }
            if (znak == EOF) {
                (void)ungetc(znak, stdin);
                spaceCount = 0; // TODO: zkontrolovat validitu
                currState = STATE_PROCESS_INDENT;
                break;
            }
            // dostali jsme na vstup znak, jdeme zpracovat indent
            currState = STATE_PROCESS_INDENT;
            (void)ungetc(znak, stdin);
            break;
        }
        case STATE_LINE_BEG_WITH_SLASH: {
            if (znak == '/') {
                currState = STATE_LINE_COMMENT;
                break;
            }
            if (znak == '*') {
                currState = STATE_BLOCK_COMMENT;
                commentDepth = 1;
                commentHasNewline = false;
                break;
            }
            g_lexerState.pendingDivide = true;
            (void)ungetc(znak, stdin);
            // jedna se o obycejne deleni, zpracujeme indent a vratime token pro deleni
            currState = STATE_PROCESS_INDENT;
            break;
        }
        case STATE_BLOCK_COMMENT: // jsme unitr blokoveho komentare
        {
            if (znak == '\n') {
                commentHasNewline = true;
                g_lexerState.currLine++;
                break;
            }
            if (znak == '/') {
                currState = STATE_BLOCK_COMMENT_SLASH;
                break;
            }
            if (znak == '*') {
                currState = STATE_BLOCK_COMMENT_STAR;
                break;
            }
            if (znak == EOF) // ERROR
            {
                return lexicalError("unterminated block comment");
            }
            break; // jiny znak, pokracujeme v ignorovani
        }
        case STATE_BLOCK_COMMENT_SLASH: {
            if (znak == '*') {
                commentDepth++;
                currState = STATE_BLOCK_COMMENT;
                break;
            }
            if (znak == '\n') {
                commentHasNewline = true;
                g_lexerState.currLine++;
                currState = STATE_BLOCK_COMMENT;
                break;
            }
            if (znak == EOF) // ERROR
            {
                return lexicalError("unterminated block comment");
            }
            if (znak == '/') {
                currState = STATE_BLOCK_COMMENT_SLASH;
                break;
            }
            currState = STATE_BLOCK_COMMENT;
            break;
        }
        case STATE_BLOCK_COMMENT_STAR: {
            if (znak == '/') {
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
            if (znak == '*') {
                // zustavame
                break;
            }
            if (znak == EOF) // ERROR
            {
                return lexicalError("unterminated block comment");
            }
            if (znak == '\n') {
                commentHasNewline = true;
                g_lexerState.currLine++;
                currState = STATE_BLOCK_COMMENT;
                break;
            }
            // znak je jiny, zpet do blok. kom
            currState = STATE_BLOCK_COMMENT;
            break;
        }
        case STATE_LINE_BEG_AFTER_BLOCK_COMMENT: // zacatek radku a zpracovany blokovy komentar
        {
            if (znak == ' ' || znak == '\t') {
                // ignore
                break;
            }
            if (znak == '/') {
                currState = STATE_LINE_BEG_WITH_SLASH;
                break;
            }
            if (znak == '\n') // prazdny radek
            {
                spaceCount = 0;
                currState = STATE_LINE_BEG;
                g_lexerState.isNewLine = true;
                g_lexerState.currLine++;
                break;
            }
            if (znak == EOF) {
                currState = STATE_START;
                spaceCount = 0;
                (void)ungetc(znak, stdin);
                break;
            }
            // neni nic z predchozich -> mame znak -> resime indent
            (void)ungetc(znak, stdin);
            currState = STATE_PROCESS_INDENT;
            break;
        }
        case STATE_PROCESS_INDENT: {

            int topIndent = g_lexerState.indentStack[g_lexerState.stackCount - 1];
            if (znak == EOF && topIndent == 0 && !g_lexerState.pendingDivide) {
                token->type = TOKEN_EOF;
                token->line = g_lexerState.currLine;
                return 0;
            }
            if (spaceCount > topIndent) {
                if (g_lexerState.stackCount >= MAX_INDENT_SIZE) {
                    return internalError("maximum indentation nesting depth exceeded");
                }
                g_lexerState.indentStack[g_lexerState.stackCount] = spaceCount;
                g_lexerState.stackCount++;
                g_lexerState.isNewLine = false;
                token->type = TOKEN_INDENT;
                token->line = g_lexerState.currLine;
                (void)ungetc(znak, stdin);
                return 0;
            } else if (spaceCount < topIndent) {
                int dedentsToEmit = 0;
                while (g_lexerState.stackCount > 0 &&
                       g_lexerState.indentStack[g_lexerState.stackCount - 1] > spaceCount) {
                    g_lexerState.stackCount--;
                    dedentsToEmit++;
                }
                if (g_lexerState.indentStack[g_lexerState.stackCount - 1] != spaceCount) {
                    return lexicalError("inconsistent dedentation level");
                }
                g_lexerState.pendingDedents = dedentsToEmit - 1;
                if (znak != EOF) {
                    g_lexerState.isNewLine = false;
                }
                token->type = TOKEN_DEDENT;
                token->line = g_lexerState.currLine;
                (void)ungetc(znak, stdin);
                return 0;
            } else { // neni indent ani dedent
                g_lexerState.isNewLine = false;
                currState = STATE_START;
                (void)ungetc(znak, stdin);
                break;
            }
        }
        case STATE_AFTER_MULTILINE_BLOCK_COMMENT: // pokud je blokovy komentar na vic radku, nesmi
                                                  // za nim nic nasledovat
        {
            if (znak == ' ' || znak == '\t') {
                break; // bily znak je v poradku
            }
            if (znak == '\n') {
                if (g_lexerState.lineHasCode && g_lexerState.bracketCount == 0) {
                    spaceCount = 0;
                    g_lexerState.lineHasCode = false;
                    g_lexerState.isNewLine = true;
                    currState = STATE_START;
                    token->type = TOKEN_EOL;
                    token->line = g_lexerState.currLine;
                    g_lexerState.currLine++;

                    return 0;
                } else { // komentar je na prazdnych radcich, ignoruje se
                    g_lexerState.lineHasCode = false;
                    g_lexerState.isNewLine = true;
                    spaceCount = 0;
                    g_lexerState.currLine++;
                    currState = STATE_START;
                    break;
                }
            }
            if (znak == EOF) {
                spaceCount = 0;
                (void)ungetc(znak, stdin);
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
            // jiny nez povoleny token, error
            return lexicalError("unexpected token after multiline block comment");
        }
        case STATE_AT_SIGN: {
            if (isalnum(znak) || znak == '_') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                break;
            }
            (void)ungetc(znak, stdin);
            if (strCompare(&dynStr, "main") == 0) {
                strFree(&dynStr);
                token->type = TOKEN_MAIN;
                token->line = g_lexerState.currLine;
                g_lexerState.lineHasCode = true;
                return 0;
            }
            return lexicalErrorFree(&dynStr, "invalid annotation, only '@main' is allowed");
        }
        case STATE_STRING_START: {
            if (znak == '"') {
                currState = STATE_STRING_SECOND_QUOTE;
                break;
            }
            if (znak == '\\') {
                currState = STATE_STRING_ESCAPE_SEQUENCE;
                break;
            }
            if (znak == '\n' || znak == '\r' || znak == EOF) {
                return lexicalErrorFree(&dynStr, "unterminated string literal");
            }
            if (strAddChar(&dynStr, (char)znak) != 0) {
                return internalErrorFree(&dynStr, "memory allocation failed");
            }
            currState = STATE_STRING;
            break;
        }
        case STATE_STRING_SECOND_QUOTE: {
            if (znak == '"') {
                currState = STATE_MULTILINE_STRING;
                break;
            }
            // nejde o uvozovku, takze mame prazdny jednoradkovy retezec
            (void)ungetc(znak, stdin);
            return finishStringToken(token, &dynStr);
        }
        case STATE_STRING: {
            if (znak == '"') { // konec stringu
                return finishStringToken(token, &dynStr);
            }
            if (znak == '\\') {
                currState = STATE_STRING_ESCAPE_SEQUENCE;
                break;
            }
            if (znak == '\n' || znak == '\r' ||
                znak == EOF) { // neni povoleno v ramci jednoradkoveho komentare
                return lexicalErrorFree(&dynStr, "unterminated string literal");
            }
            if (strAddChar(&dynStr, (char)znak) != 0) {
                return internalErrorFree(&dynStr, "memory allocation failed");
            }
            break;
        }
        case STATE_STRING_ESCAPE_SEQUENCE: { // escape sekvence
            char escapedChar;
            switch (znak) { // podporovány jsou escape sekvence \\, \", \n, \r a \t.
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
            default: // nepodporovany znak
                return lexicalErrorFree(&dynStr, "invalid escape sequence in string literal");
            }
            if (strAddChar(&dynStr, escapedChar) != 0) {
                return internalErrorFree(&dynStr, "memory allocation failed");
            }
            currState = STATE_STRING;
            break;
        }
        case STATE_MULTILINE_STRING: {
            if (znak == EOF) { // neukonceny string
                return lexicalErrorFree(&dynStr, "unterminated multiline string literal");
            }
            if (znak == '"') {
                currState = STATE_MULTILINE_QUOTE1;
                break;
            }
            if (znak == '\n') {
                g_lexerState.currLine++;
            }
            if (strAddChar(&dynStr, (char)znak) != 0) {
                return internalErrorFree(&dynStr, "memory allocation failed");
            }
            break;
        }

        case STATE_MULTILINE_QUOTE1: {
            if (znak == EOF) {
                return lexicalErrorFree(&dynStr, "unterminated multiline string literal");
            }
            if (znak == '"') {
                currState = STATE_MULTILINE_QUOTE2;
                break;
            }
            if (znak == '\n') {
                g_lexerState.currLine++;
            }
            if (strAddChar(&dynStr, '"') != 0 ||
                strAddChar(&dynStr, (char)znak) != 0) { // pridame uvozovku z predchoziho nacteni
                return internalErrorFree(&dynStr, "memory allocation failed");
            }
            currState = STATE_MULTILINE_STRING;
            break;
        }
        case STATE_MULTILINE_QUOTE2: {
            if (znak == EOF) { // chyba
                return lexicalErrorFree(&dynStr, "unterminated multiline string literal");
            }
            if (znak == '"') {
                currState = STATE_MULTILINE_END; // tri uvozovky za sebou, jdeme do posledniho stavu
                break;
            }
            if (znak == '\n') {
                g_lexerState.currLine++;
            }
            if (strAddChar(&dynStr, '"') != 0 ||
                strAddChar(&dynStr, '"') !=
                    0 || // jiny znak, musime vratit predchozi nactene uvozovky a znak do stringu
                strAddChar(&dynStr, (char)znak) != 0) {
                return internalErrorFree(&dynStr, "memory allocation failed");
            }
            currState = STATE_MULTILINE_STRING;
            break;
        }
        case STATE_MULTILINE_END: {

            if (znak == '"') {
                if (strAddChar(&dynStr, '"') != 0) {
                    return internalErrorFree(&dynStr, "memory allocation failed");
                }
                break;
            }
            (void)ungetc(znak, stdin);
            return finishStringToken(token, &dynStr);
        }
        default: {
            break;
        }
        }
    }
}
