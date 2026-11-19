/**
 * @file    parser.h
 * @brief   Header file for recursive descend syntax parsing and AST construction
 * @author  Jaroslav Renda (xrendaj00), Jakub Poláček (xpolacj00)
 * @details project_IFJ2026
 */

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
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

bool parseProg(currentToken *cur, ASTNode **outProgram);
bool parseFooList(currentToken *cur, ASTNode **outFuncList);
bool parseFooDec(currentToken *cur, ASTNode **outFunc);
bool parseOptEol(currentToken *cur);
bool parseParams(currentToken *cur, ASTNode **outParam);
bool parseNextParam(currentToken *cur, ASTNode **outParam);
bool parseAfterCom(currentToken *cur, ASTNode **outParam);
bool parseFooBody(currentToken *cur, ASTNode **outBody);
bool parseBlock(currentToken *cur, ASTNode **outBlock);
bool parseStList(currentToken *cur, ASTNode **outStmt);
bool parseStNext(currentToken *cur, ASTNode **outNextStmt);
bool parseStRest(currentToken *cur, ASTNode **outNextStmt);
bool parseStat(currentToken *cur, ASTNode **outStmt);
bool parseBlockStat(currentToken *cur, ASTNode **outStmt);
bool parseRetVal(currentToken *cur, ASTNode **outExpr);
bool parseOptType(currentToken *cur, dataType *outType);
bool parseType(currentToken *cur, dataType *outType);
bool parseStAfterId(currentToken *cur, ASTNode **outStmt);

#endif // !PARSER_H
