/**
 * @file    lexer.h
 * @author  Jakub Zenata (xzenatj00)
 * @brief   header file of lexer
 * @date    2026-11-19
 * @details project_IFJ2026
 */

#ifndef LEXER_H
#define LEXER_H
#include <stdbool.h>
#include <stdio.h>

#define KEYWORDS_COUNT 22

enum automatState {
    STATE_START,
    STATE_IDENTIFIER,
    STATE_FULL_NUMBER,                // koncovy
    STATE_FULL_NUMBER_UNDERSCORE,     // nekoncovy
    STATE_ZERO_NUMBER,                // koncovy
    STATE_DECIMAL_POINT,              // nacetli jsme . cekame na cislo, jinak error
    STATE_DECIMAL_NUMBER,             // koncovy
    STATE_DECIMAL_NUMBER_UNDERSCORE,  // nekoncovy
    STATE_EXPONENT_E,                 // nacetli jsme e, cekame na znamenko/cislo , neni koncovy
    STATE_EXPONENT_SIGN,              // nacetli jsme e a znamenko
    STATE_EXPONENT_NUMBER,            // cislo
    STATE_EXPONENT_NUMBER_UNDERSCORE, // cislo
    STATE_DOUBLE_SUFFIX,              // nacetli jsme d, konecny stav pro double

    STATE_SLASH,
    STATE_LINE_COMMENT, // radkovy komentar
    STATE_ANGLE_BRACKET_LESSER,
    STATE_ANGLE_BRACKET_BIGGER,
    STATE_EQUAL,
    STATE_EXCLAMATION_MARK,
    STATE_LINE_BEG,
    STATE_LINE_BEG_WITH_SLASH, // moznost vyskytu radkoveho komentare.pokud ano, -> prazdny radek

    STATE_LINE_BEG_AFTER_BLOCK_COMMENT, // jsme na zacatku radku a mame detekovany blokovy komentar
    STATE_BLOCK_COMMENT,                // uvnitr /*
    STATE_BLOCK_COMMENT_SLASH,          // uvnitr /*   , dostanem /
    STATE_BLOCK_COMMENT_STAR,           // uvnitr /*   , dostanem *

    STATE_PROCESS_INDENT,
    STATE_AFTER_MULTILINE_BLOCK_COMMENT,
    STATE_AT_SIGN,

    STATE_STRING_START,
    STATE_STRING_SECOND_QUOTE,
    STATE_STRING,
    STATE_STRING_ESCAPE_SEQUENCE,

};

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
    TOKEN_PERIOD,
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
    TOKEN_MAIN,
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
        char *stringVal;
        double doubleVal;
        int intVal;
        keyword keywordVal;
    } data;

} Token;

/**
 * @brief Reads sequence of characters from stdin, processes it, filters out
 * comments and whitespaces and returns a token
 * @param token Pointer to where the output token will be stored
 * @return int Returns 0 on succesfull token generation, or 1 in case of lexical
 * analysis error
 */

int getToken(Token *token);

/**
 * @brief Initialization function of lexer state, must be called before first
 * call of getToken
 */
void lexerStateInit();

#endif
