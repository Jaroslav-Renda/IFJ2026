/**
 * @file    lexer.c
 * @author  Jakub Zenata (xzenatj00)
 * @brief   lexer implementation
 * @date    2026-11-19
 * @details project_IFJ2026
 */

#include "lexer.h"
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

int getToken(Token *token) {
    if (!g_lexerStateInitialized) {
        return 99;
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
            return 1; // nezname znaky
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
                g_lexerState.lineHasCode = true;
                strFree(&dynStr);
            } else {
                token->type = TOKEN_IDENTIFIER;
                g_lexerState.lineHasCode = true;
                token->data.identifier = malloc(strlen(dynStr.str) + 1);
                if (token->data.identifier == NULL) {
                    strFree(&dynStr);
                    return 99;
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

            return 1; // error TODO dodelat errory
        }
        case STATE_LINE_BEG: {
            if (znak == ' ') {
                spaceCount++;
                break;
            }
            if (znak == '\t') // jedna se o prazdny radek
            {
                // lexikalni chyba
                return 1;
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
                return 1;
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
                return 1;
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
                return 1;
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
                    return 99;
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
                    return 1;
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
            return 1;
        }

        default: {
            break;
        }
        }
    }
}
