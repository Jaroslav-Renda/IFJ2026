/**
 * @file    lexer.c
 * @author  Jakub Zenata (xzenatj00)
 * @brief   lexer implementation
 * @date    2026-11-16
 * @details project_IFJ2026
 */

#include "lexer.h"
#include "dynamicString.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INDENT_SIZE 50

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

} lexerState;
typedef struct TKeywordPair {
    const char *str;
    keyword keywordVal;
} KeywordPair; // struct used to compare string with keywords

static lexerState g_lexerState; // private lexer state,
// used primarelly for parsing indent/dedent blocks
static bool g_lexerStateInitialized;

void lexerStateInit() {
    g_lexerStateInitialized = true;
    g_lexerState.currLine = 1;
    g_lexerState.indentStack[0] = 0;
    g_lexerState.stackCount = 1;
    g_lexerState.isNewLine = true;
    g_lexerState.pendingDedents = 0;
    g_lexerState.bracketCount = 0;
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

int getToken(Token *token) {
    int currState = STATE_START;
    DynamicString dynStr;
    while (true) {
        int znak = getchar();
        switch (currState) {
        case STATE_START: { // pocatecni stav

            if (znak == EOF) {
                token->type = TOKEN_EOF;
                token->line = g_lexerState.currLine;
                return 0;
            }
            if (znak == '\n') {
                if (g_lexerState.bracketCount == 0) {
                    token->type = TOKEN_EOL;
                    token->line = g_lexerState.currLine;
                    g_lexerState.currLine++;
                    return 0; // newline je uvnitr zavorek, pokracujeme ve cteni
                }
                break;
            }
            if (znak == '(') {
                token->type = TOKEN_LEFT_BRACKET;
                token->line = g_lexerState.currLine;
                g_lexerState.bracketCount++;
                return 0;
            }
            if (znak == ')') {
                token->type = TOKEN_RIGHT_BRACKET;
                token->line = g_lexerState.currLine;
                g_lexerState.bracketCount--;
                return 0;
            }
            if (znak == ';') {
                token->type = TOKEN_SEMICOLON;
                token->line = g_lexerState.currLine;
                return 0;
            }
            if (znak == ':') {
                token->type = TOKEN_COLON;
                token->line = g_lexerState.currLine;
                return 0;
            }
            if (znak == '.') {
                token->type = TOKEN_PERIOD;
                token->line = g_lexerState.currLine;
                return 0;
            }
            if (znak == ',') {
                token->type = TOKEN_COMMA;
                token->line = g_lexerState.currLine;
                return 0;
            }
            if (znak == '+') {
                token->type = TOKEN_PLUS;
                token->line = g_lexerState.currLine;
                return 0;
            }
            if (znak == '-') {
                token->type = TOKEN_MINUS;
                token->line = g_lexerState.currLine;
                return 0;
            }
            if (znak == '*') {
                token->type = TOKEN_MULTIPLY;
                token->line = g_lexerState.currLine;
                return 0;
            }
            if (znak == '/') {
                currState = STATE_SLASH;
                break;
            }
            if (znak == '<') {
                currState = STATE_ANGLE_BRACKET_LESSER;
            }
            if (znak == '>') {
                currState = STATE_ANGLE_BRACKET_BIGGER;
            }
            if (znak == '=') {
                currState = STATE_EQUAL;
            }
            if (znak == '!') {
                currState = STATE_EXCLAMATION_MARK;
            }
            if (isalpha(znak) || znak == '_') {
                if (strInit(&dynStr) != 0) {
                    return 99;
                }
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return 99;
                }
                currState = STATE_IDENTIFIER; // identifikator
                break;
            }
            if (znak == '0') {
                if (strInit(&dynStr) != 0) {
                    return 99;
                }
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return 99;
                }
                currState = STATE_ZERO_NUMBER;
                break;
            }
            if (znak > '0' && znak <= '9') {
                if (strInit(&dynStr) != 0) {
                    return 99;
                }
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return 99;
                }
                currState = STATE_FULL_NUMBER; // cislo
                break;
            }
            break;
        }
        case STATE_IDENTIFIER: // identifikator
        {
            if (isalnum(znak) || znak == '_') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return 99;
                }
                break;
            }
            (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy;
            keyword keywordVal;
            if (checkForKeyword(&dynStr, &keywordVal)) {
                token->type = TOKEN_KEYWORD;
                token->data.keywordVal = keywordVal;
                strFree(&dynStr);
            } else {
                token->type = TOKEN_IDENTIFIER;
                token->data.identifier = malloc(strlen(dynStr.str) + 1);
                if (token->data.identifier == NULL) {
                    strFree(&dynStr);
                    return 99;
                }
                strcpy(token->data.identifier, dynStr.str);
                strFree(&dynStr);
            }
            return 0; // validni token, konec cele funkce
        }
        case STATE_ZERO_NUMBER: {
            if (znak == '.') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return 99;
                }
                currState = STATE_DECIMAL_POINT;
            } else if (znak == 'e' || znak == 'E') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return 99;
                }
                currState = STATE_EXPONENT_E;
            } else if (znak == 'd' || znak == 'D') {
                currState = STATE_DOUBLE_SUFFIX;
            } else if (isdigit(znak)) { // lex chyba, cele cislo nesmi zacinat nulou
                strFree(&dynStr);
                return 1;
            } else {
                (void)ungetc(znak, stdin);
                token->type = TOKEN_INT;
                char *endptr;
                token->data.intVal = (int)strtol(dynStr.str, &endptr, 10);
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
                    return 99;
                }
                currState = STATE_FULL_NUMBER; // zustavame ve stejnem stavu

            } else if (znak == 'e' || znak == 'E') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return 99;
                }
                currState = STATE_EXPONENT_E;

            } else if (znak == 'd' || znak == 'D') {
                currState = STATE_DOUBLE_SUFFIX;

            } else if (znak == '.') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return 99;
                }
                currState = STATE_DECIMAL_POINT;

            } else {
                (void)ungetc(znak, stdin);
                token->type = TOKEN_INT;
                char *endptr;
                token->data.intVal = (int)strtol(dynStr.str, &endptr, 10);
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
                    return 99;
                }
                currState = STATE_FULL_NUMBER;
            } else {
                strFree(&dynStr);
                return 1;
            }
            break;
        }

        case STATE_DECIMAL_POINT: // cislo
        {
            if (znak >= '0' && znak <= '9') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return 99;
                }
                currState = STATE_DECIMAL_NUMBER;

            } else {
                strFree(&dynStr);
                return 1;
            }
            break;
        }
        case STATE_DECIMAL_NUMBER: {
            if (znak == '_') {
                currState = STATE_DECIMAL_NUMBER_UNDERSCORE;
            } else if (znak >= '0' && znak <= '9') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return 99;
                }
                currState = STATE_DECIMAL_NUMBER;
            } else if (znak == 'e' || znak == 'E') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return 99;
                }
                currState = STATE_EXPONENT_E;

            } else if (znak == 'd' || znak == 'D') {
                currState = STATE_DOUBLE_SUFFIX;

            } else {
                (void)ungetc(znak, stdin);
                token->type = TOKEN_DOUBLE;
                char *endptr;
                token->data.doubleVal = strtod(dynStr.str, &endptr);
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
                    return 99;
                }
                currState = STATE_DECIMAL_NUMBER;
            } else {
                strFree(&dynStr);
                return 1;
            }
            break;
        }
        case STATE_EXPONENT_E: {
            if (znak == '-') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return 99;
                }
                currState = STATE_EXPONENT_SIGN;

            } else if (znak >= '0' && znak <= '9') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return 99;
                }
                currState = STATE_EXPONENT_NUMBER;
            } else {
                strFree(&dynStr);
                return 1;
            }
            break;
        }
        case STATE_EXPONENT_SIGN: {
            if (znak >= '0' && znak <= '9') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return 99;
                }
                currState = STATE_EXPONENT_NUMBER;
            } else {
                strFree(&dynStr);
                return 1; // error
            }
            break;
        }
        case STATE_EXPONENT_NUMBER: {
            if (znak == '_') {
                currState = STATE_EXPONENT_NUMBER_UNDERSCORE;
            } else if (znak >= '0' && znak <= '9') {
                if (strAddChar(&dynStr, (char)znak) != 0) {
                    return 99;
                }
                currState = STATE_DECIMAL_NUMBER;
            } else if (znak == 'd' || znak == 'D') {
                currState = STATE_DOUBLE_SUFFIX;

            } else {
                (void)ungetc(znak, stdin);
                token->type = TOKEN_DOUBLE;
                char *endptr;
                token->data.doubleVal = strtod(dynStr.str, &endptr);
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
                    return 99;
                }
                currState = STATE_EXPONENT_NUMBER;
            } else {
                strFree(&dynStr);
                return 1;
            }
            break;
        }
        case STATE_DOUBLE_SUFFIX: {
            (void)ungetc(znak, stdin);
            token->type = TOKEN_DOUBLE;
            char *endptr;
            token->data.doubleVal = strtod(dynStr.str, &endptr);
            strFree(&dynStr);
            return 0;
        }
        case STATE_SLASH: {
            if (znak == '/') {
                currState = STATE_LINE_COMMENT; // stav pro radkovy komentar
            } else {
                (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy
                token->type = TOKEN_DIVIDE;
                token->line = g_lexerState.currLine;
                return 0;
            }
            break;
        }
        case STATE_LINE_COMMENT: // radkovy komentar
        {
            if (znak == '\n' || znak == EOF) {
                g_lexerState.currLine++;
                token->type = TOKEN_EOL;
                return 0;
            }
            currState = STATE_LINE_COMMENT;
            break;
        }
        default: {
            break;
        }
        case STATE_ANGLE_BRACKET_LESSER: {
            if (znak == '=') {
                token->type = TOKEN_COMPARE_LESSER_EQUAL;
                token->line = g_lexerState.currLine;
                return 0;
            }
            (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy
            token->type = TOKEN_COMPARE_SMALLER_THAN;
            token->line = g_lexerState.currLine;
            return 0;
        }
        case STATE_ANGLE_BRACKET_BIGGER: {
            if (znak == '=') {
                token->type = TOKEN_COMPARE_BIGGER_EQUAL;
                token->line = g_lexerState.currLine;
                return 0;
            }
            (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy
            token->type = TOKEN_COMPARE_BIGGER_THAN;
            token->line = g_lexerState.currLine;
            return 0;
        }
        case STATE_EQUAL: {

            if (znak == '=') {
                token->type = TOKEN_EQUAL_TO;
                token->line = g_lexerState.currLine;
                return 0;
            }
            (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy
            token->type = TOKEN_EQUAL_SIGN;
            token->line = g_lexerState.currLine;
            return 0;
        }
        case STATE_EXCLAMATION_MARK: {
            if (znak == '=') {
                token->type = TOKEN_NOT_EQUAL_TO;
                token->line = g_lexerState.currLine;
                return 0;
            }
            (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy

            return 1; // error TODO dodelat errory
        }
        }
    }
}
