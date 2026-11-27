/**
 * @file    lexer.h
 * @author  Jakub Zenata (xzenatj00)
 * @brief   header file of lexer
 * @details project_IFJ2026
 */

#ifndef LEXER_H
#define LEXER_H

#define KEYWORDS_COUNT 22

enum automatState {
    STATE_START,
    STATE_IDENTIFIER,
    STATE_FULL_NUMBER,
    STATE_FULL_NUMBER_UNDERSCORE,
    STATE_ZERO_NUMBER,
    STATE_DECIMAL_POINT,
    STATE_DECIMAL_NUMBER,
    STATE_DECIMAL_NUMBER_UNDERSCORE,
    STATE_EXPONENT_E,
    STATE_EXPONENT_SIGN,
    STATE_EXPONENT_NUMBER,
    STATE_EXPONENT_NUMBER_UNDERSCORE,
    STATE_DOUBLE_SUFFIX,

    STATE_SLASH,
    STATE_LINE_COMMENT,
    STATE_ANGLE_BRACKET_LESSER,
    STATE_ANGLE_BRACKET_BIGGER,
    STATE_EQUAL,
    STATE_EXCLAMATION_MARK,
    STATE_LINE_BEG,
    STATE_LINE_BEG_WITH_SLASH,

    STATE_LINE_BEG_AFTER_BLOCK_COMMENT,
    STATE_BLOCK_COMMENT,
    STATE_BLOCK_COMMENT_SLASH,
    STATE_BLOCK_COMMENT_STAR,

    STATE_PROCESS_INDENT,
    STATE_AFTER_MULTILINE_BLOCK_COMMENT,
    STATE_AT_SIGN,

    STATE_STRING_START,
    STATE_STRING_SECOND_QUOTE,
    STATE_STRING,
    STATE_STRING_ESCAPE_SEQUENCE,
    STATE_MULTILINE_STRING,
    STATE_MULTILINE_QUOTE1,
    STATE_MULTILINE_QUOTE2,
    STATE_MULTILINE_END,

    STATE_PERIOD
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
