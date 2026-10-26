/**
 * @file    lexer.h
 * @author  Jakub Zenata (xzenatj00)
 * @brief   header file of lexer
 * @date    2026-10-06
 */

#ifndef LEXER_H
#define LEXER_H
#define MAX_INDENT_SIZE 50
#include <stdbool.h>
#include <stdio.h>

/**
 * @brief all supported tokens
 */

typedef enum {
    TOKEN_KEYWORD,
    TOKEN_INT,
    TOKEN_DOUBLE,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_EQUAL_SIGN,
    TOKEN_EOL,
    TOKEN_EOF,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_COMPARE_SMALLER_THAN,
    TOKEN_COMPARE_BIGGER_THAN,
    TOKEN_COMPARE_LESSER_EQUAL,
    TOKEN_COMPARE_BIGGER_EQUAL,
    TOKEN_EQUAL_TO,
    TOKEN_NOT_EQUAL_TO,
    TOKEN_INDENT,
    TOKEN_DEDENT,
} tokenType;

/**
 * @brief all supported keywords
 */
typedef enum {
    KEYWORD_DEF,
    KEYWORD_DO,
    KEYWORD_DOUBLE,
    KEYWORD_ELSE,
    KEYWORD_IF,
    KEYWORD_IMPORT,
    KEYWORD_INT,
    KEYWORD_RETURN,
    KEYWORD_STRING,
    KEYWORD_THEN,
    KEYWORD_UNIT,
    KEYWORD_VAL,
    KEYWORD_VAR,
    KEYWORD_WHILE,
    KEYWORD_BOOLEAN,
    KEYWORD_CASE,
    KEYWORD_CLASS,
    KEYWORD_EXTENSION,
    KEYWORD_FOR,
    KEYWORD_LIST,
    KEYWORD_MATCH,
    KEYWORD_TYPE,
} keyword;
/**
 * @brief struct representing a token
 */
typedef struct {
    tokenType type;
    int line; // line on which the token is located

    /**
     * @brief union holding the attribute of token
     *  only one attribute is used, corresponding with token type
     */
    union {
        char *identifier;
        double doubleVal;
        int intVal;
        keyword keywordVal;
    } data;

} Token;

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

} lexerState;

/**
 * @brief Reads sequence of characters from stdin, processes it, filters out
 * comments and whitespaces and returns a token
 * @param token Pointer to where the output token will be stored
 * @param lexerState Pointer to the structure representing current state of the
 * lexer, used to help with processing indentation and return curr line number
 * @return int Returns 0 on succesfull token generation, or 1 in case of lexical
 * analysis error
 */

int getToken(Token *token, lexerState *state);

/**
 * @brief Initialization function of lexer state
 * @param lexerState Pointer to lexerState struct, which will be inicialized
 */
void lexerStateInit(lexerState *state);

#endif
