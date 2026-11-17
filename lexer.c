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
    int bracketCount;

} lexerState;

static lexerState g_lexerState;
static bool g_lexerStateInitialized;

void lexerStateInit() {
    g_lexerState.currLine = 1;
    g_lexerState.indentStack[0] = 0;
    g_lexerState.stackCount = 1;
    g_lexerState.isNewLine = true;
    g_lexerState.pendingDedents = 0;
    g_lexerStateInitialized = true;
    g_lexerState.bracketCount = 0;
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
                g_lexerState.currLine++;
                token->type = TOKEN_EOL;
                return 0;
            }
            if (znak == '(') {
                token->type = TOKEN_LEFT_BRACKET;
                token->line = g_lexerState.currLine;
                return 0;
            }
            if (znak == ')') {
                token->type = TOKEN_RIGHT_BRACKET;
                token->line = g_lexerState.currLine;
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
                znak = getchar();
                if (znak == '/') {
                    currState = STATE_LINE_COMMENT; // stav pro radkovy komentar
                } else {
                    (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy
                    token->type = TOKEN_DIVIDE;
                    token->line = g_lexerState.currLine;
                    return 0;
                }
            }
            if (znak == '<') {
                znak = getchar();
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
            if (znak == '>') {
                znak = getchar();
                if (znak == '=') {
                    token->type = TOKEN_COMPARE_BIGGER_EQUAL;
                    token->line = g_lexerState.currLine;
                    return 0;
                }
                (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy
                token->type = TOKEN_COMPARE_SMALLER_THAN;
                token->line = g_lexerState.currLine;
                return 0;
            }
            if (znak == '=') {
                znak = getchar();
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
            if (znak == '!') {
                znak = getchar();
                if (znak == '=') {
                    token->type = TOKEN_NOT_EQUAL_TO;
                    token->line = g_lexerState.currLine;
                    return 0;
                }
                (void)ungetc(znak, stdin); // pretypovano na void kvuli clang tidy
                // error
                return 1;
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
                currState = STATE_ZERO_NUMBER;
            }
            if (znak > '0' && znak <= '9') {
                currState = STATE_DECIMAL_NUMBER; // cislo
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
            // volani funkce pro kontrolu keywordu
            token->type = TOKEN_IDENTIFIER;
            token->data.identifier = dynStr.str;
            return 0; // validni token, konec cele funkce
        }
        case STATE_DECIMAL_NUMBER: // cislo
        {
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
        }
    }
}
