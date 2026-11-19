/**
 * @file    parser.h
 * @brief   Header file for recursive descend syntax parsing
 * @author  Jaroslav Renda (xrendaj00)
 * @details project_IFJ2026
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

/**
 * @brief A wrapper structure for current loaded token
 */
typedef struct {
    /// The token
    Token token;
} currentToken;

/**
 * @brief A wrapper structure for lookahead token
 */
typedef struct {
    /// Was the lookahead token used
    bool isEmpty;
    /// The token
    Token token;
} lookaheadToken;

/**
 * @brief Parses the root program structure (<PROG>).
 *
 * Implements LL(1) rule:
 * - Rule 1: <PROG> => "import" "ifj2026" "." "*" "EOL" <FOO-LIST> "EOF"
 *
 * Initializes the first lookahead token, verifies the mandatory prolog,
 * delegates function declarations to parseFooList(), and checks for EOF.
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if the entire program is syntactically valid, false otherwise.
 */
bool parseProg(currentToken *cur);

/**
 * @brief Parses the sequence of top-level function declarations (<FOO-LIST>).
 *
 * Implements LL(1) rules:
 * - Rule 2: <FOO-LIST> => <FOO-DEC> <FOO-LIST>
 * - Rule 3: <FOO-LIST> => e
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if the function list (or epsilon at EOF) is valid, false otherwise.
 */
bool parseFooList(currentToken *cur);

/**
 * @brief Parses a single function or main entry-point declaration (<FOO-DEC>).
 *
 * Implements LL(1) rules:
 * - Rule 4: <FOO-DEC> => "@main" <OPT-EOL> "def" "id" "(" <OPT-EOL> <PARAMS>")" <OPT-TYPE> "="
 * <FOO-BODY>
 * - Rule 5: <FOO-DEC> => "def" "id" "(" <OPT-EOL> <PARAMS> ")" <OPT-TYPE> "=" <FOO-BODY>
 *
 * A main function should not have any parameters and the type can only be Unit, this is enforced
 * by the semantic analysis and the parser allows invalid statements.
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if the function header and body are valid, false otherwise.
 */
bool parseFooDec(currentToken *cur);

/**
 * @brief Parses an optional end-of-line token (<OPT-EOL>).
 *
 * Implements LL(1) rules:
 * - Rule 6: <OPT-EOL> => "EOL"
 * - Rule 7: <OPT-EOL> => e
 *
 * Consumes a single EOL token if present; otherwise leaves the current token
 * untouched for subsequent rules.
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if an EOL or a valid FOLLOW(<OPT-EOL>) token is found, false otherwise.
 */
bool parseOptEol(currentToken *cur);

/**
 * @brief Parses the function parameter list (<PARAMS>).
 *
 * Implements LL(1) rules:
 * - Rule 8: <PARAMS> => e
 * - Rule 9: <PARAMS> => "id" <OPT-EOL> ":" <OPT-EOL> <TYPE> <OPT-EOL> <NEXT-PARAM>
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if the parameter list (or empty list before ')') is valid, false otherwise.
 */
bool parseParams(currentToken *cur);

/**
 * @brief Parses the continuation of a parameter list (<NEXT-PARAM>).
 *
 * Implements LL(1) rules:
 * - Rule 10: <NEXT-PARAM> => e
 * - Rule 11: <NEXT-PARAM> => "," <AFTER-COM>
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if the parameter continuation or closing parenthesis follows, false otherwise.
 */
bool parseNextParam(currentToken *cur);

/**
 * @brief Parses the parameter following a comma separator (<AFTER-COM>).
 *
 * Implements LL(1) rules:
 * - Rule 12: <AFTER-COM> => "id" <OPT-EOL> ":" <OPT-EOL> <TYPE> <OPT-EOL> <NEXT-PARAM>
 * - Rule 13: <AFTER-COM> => "EOL" <PARAMS>
 *
 * Supports trailing commas before a closing parenthesis when followed by EOL.
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if the tokens after the comma are valid, false otherwise.
 */
bool parseAfterCom(currentToken *cur);

/**
 * @brief Parses the body of a function declaration (<FOO-BODY>).
 *
 * Implements LL(1) rules:
 * - Rule 14: <FOO-BODY> => "EOL" <BLOCK>
 * - Rule 15: <FOO-BODY> => <EXP> "EOL"
 *
 * Handles both multi-line indented blocks and single-line expression bodies.
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if the function body is syntactically valid, false otherwise.
 */
bool parseFooBody(currentToken *cur);

/**
 * @brief Parses an indented block of statements (<BLOCK>).
 *
 * Implements LL(1) rule:
 * - Rule 16: <BLOCK> => "Indent" <ST-LIST> "Dedent"
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if the block is properly indented, contains valid statements, and dedents, false
 * otherwise.
 */
bool parseBlock(currentToken *cur);

/**
 * @brief Parses a non-empty list of statements inside a block (<ST-LIST>).
 *
 * Implements LL(1) rules:
 * - Rule 17: <ST-LIST> => <STAT> <ST-NEXT>
 * - Rule 18: <ST-LIST> => <BLOCK-STAT> <ST-REST>
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if the statement list is valid, false otherwise.
 */
bool parseStList(currentToken *cur);

/**
 * @brief Parses the mandatory delimiter after a simple statement (<ST-NEXT>).
 *
 * Implements LL(1) rules:
 * - Rule 19: <ST-NEXT> => "EOL" <ST-REST>
 * - Rule 20: <ST-NEXT> => ";" <OPT-EOL> <ST-REST>
 *
 * Allows multiple statements on a single line separated by semicolons.
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if a valid statement delimiter and remaining statements follow, false otherwise.
 */
bool parseStNext(currentToken *cur);

/**
 * @brief Parses the remaining statements in a block (<ST-REST>).
 *
 * Implements LL(1) rules:
 * - Rule 21: <ST-REST> => e
 * - Rule 22: <ST-REST> => <STAT> <ST-NEXT>
 * - Rule 23: <ST-REST> => <BLOCK-STAT> <ST-REST>
 *
 * Terminates via epsilon transition when a Dedent token is encountered.
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if the rest of the block is syntactically valid, false otherwise.
 */
bool parseStRest(currentToken *cur);

/**
 * @brief Parses a single-line simple statement (<STAT>).
 *
 * Implements LL(1) rules:
 * - Rule 24: <STAT> => "val" "id" <OPT-TYPE> "=" <EXP>
 * - Rule 25: <STAT> => "var" "id" <OPT-TYPE> "=" <EXP>
 * - Rule 26: <STAT> => <EXP>
 * - Rule 27: <STAT> => "return" <RET-VAL>
 * - Rule 38: <STAT> => "id" <ST-AFTER-ID>
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if the simple statement is valid, false otherwise.
 */
bool parseStat(currentToken *cur);

/**
 * @brief Parses a compound control-flow block statement (<BLOCK-STAT>).
 *
 * Implements LL(1) rules:
 * - Rule 28: <BLOCK-STAT> => "if" <EXP> "then" "EOL" <BLOCK> "else" "EOL" <BLOCK>
 * - Rule 29: <BLOCK-STAT> => "while" <EXP> "do" "EOL" <BLOCK>
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if the conditional or loop statement is valid, false otherwise.
 */
bool parseBlockStat(currentToken *cur);

/**
 * @brief Parses the optional return value expression (<RET-VAL>).
 *
 * Implements LL(1) rules:
 * - Rule 30: <RET-VAL> => e
 * - Rule 31: <RET-VAL> => <EXP>
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if a valid expression or statement delimiter ("EOL", ";") follows, false otherwise.
 */
bool parseRetVal(currentToken *cur);

/**
 * @brief Parses an optional type annotation (<OPT-TYPE>).
 *
 * Implements LL(1) rules:
 * - Rule 32: <OPT-TYPE> => ":" <TYPE>
 * - Rule 33: <OPT-TYPE> => e
 *
 * Used in variable declarations (val/var) and function return types before '='.
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if a valid type annotation or '=' token is found, false otherwise.
 */
bool parseOptType(currentToken *cur);

/**
 * @brief Parses a built-in data type keyword (<TYPE>).
 *
 * Implements LL(1) rules:
 * - Rule 34: <TYPE> => "int"
 * - Rule 35: <TYPE> => "double"
 * - Rule 36: <TYPE> => "string"
 * - Rule 37: <TYPE> => "Unit"
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if the current token represents a valid data type, false otherwise.
 */
bool parseType(currentToken *cur);

/**
 * @brief Parses a statement after identificator token (ST-AFTER-ID).
 *
 * Implements LL(1) rules:
 * - Rule 39: <ST-AFTER-ID> => "=" <EXP>
 * - Rule 40: <ST-AFTER-ID> => <EXP>[1]
 *
 *   [1] needs to parse a lookahead token
 *
 * @param[in,out] cur Pointer to the current token structure.
 * @return true if the current token represents a valid continuation after id, false otherwise.
 */
bool parseStAfterId(currentToken *cur);

#endif // !PARSER_H
