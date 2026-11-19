/**
 * @file    parser.c
 * @brief   Implementation of recursive descend syntax parsing and AST construction
 * @author  Jaroslav Renda (xrendaj00), Jakub Poláček (xpolacj00)
 * @details project_IFJ2026
 */

#include "parser.h"
#include "ast.h"
#include "error.h"
#include "lexer.h"
#include "precedence.h"
#include <stdlib.h>
#include <string.h>

// helper function for getting next token with lexer error handling
static bool nextToken(currentToken *cur) { return (getToken(&cur->token) != 1); }

// helper function checking correct token type and loading next token
static bool expectToken(currentToken *cur, tokenType expectedType, const char *errMsg) {
    if (cur->token.type != expectedType) {
        errorParser(cur->token.line, errMsg);
        return false;
    }
    return nextToken(cur);
}

// helper function checking correct keyword type and loading next token
static bool expectKeyword(currentToken *cur, keyword expectedKey, const char *errMsg) {
    if (cur->token.type != TOKEN_KEYWORD || cur->token.data.keywordVal != expectedKey) {
        errorParser(cur->token.line, errMsg);
        return false;
    }
    return nextToken(cur);
}

static bool isLookaheadExpressionFirst(currentToken *cur) {
    switch (cur->token.type) {
    case TOKEN_LEFT_BRACKET:
    case TOKEN_SEMICOLON:
    case TOKEN_EOL:
    case TOKEN_EOF:
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_MULTIPLY:
    case TOKEN_DIVIDE:
    case TOKEN_COMPARE_BIGGER_EQUAL:
    case TOKEN_COMPARE_BIGGER_THAN:
    case TOKEN_COMPARE_LESSER_EQUAL:
    case TOKEN_COMPARE_SMALLER_THAN:
    case TOKEN_EQUAL_TO:
    case TOKEN_NOT_EQUAL_TO:
        return true;
    default:
        return false;
    }
}

bool parseStAfterId(currentToken *cur, ASTNode **outStmt) {
    lookaheadToken lat;
    lat.token = cur->token;
    lat.isEmpty = true;

    if (!nextToken(cur)) {
        free(lat.token.data.identifier);
        return false;
    }

    // Rule 39: <ST-AFTER-ID> => "=" <EXP> (Assignment: NODE_ASSIGN)
    if (cur->token.type == TOKEN_EQUAL_SIGN) {
        if (!nextToken(cur)) {
            free(lat.token.data.identifier);
            return false;
        }

        ASTNode *assignNode = createNode(NODE_ASSIGN);
        if (assignNode == NULL) {
            free(lat.token.data.identifier);
            errorInternal(cur->token.line, "Memory allocation failed for NODE_ASSIGN");
            return false;
        }
        assignNode->data.assign.targetName = lat.token.data.identifier;
        *outStmt = assignNode;

        return parseExpression(cur, NULL, &assignNode->data.assign.expr);
    }

    // Rule 40: <ST-AFTER-ID> => <EXP> (Expression / function call starting with id)
    if (isLookaheadExpressionFirst(cur)) {
        lat.isEmpty = false;
        return parseExpression(cur, &lat, outStmt);
    }

    free(lat.token.data.identifier);
    errorParser(cur->token.line, "Syntax error: unexpected token after identificator in statement");
    return false;
}

// helper function -> token can be at the start of an expression
static bool isFirstExp(currentToken *cur) {
    switch (cur->token.type) {
    case TOKEN_IDENTIFIER:
    case TOKEN_LEFT_BRACKET:
    case TOKEN_MINUS:
    case TOKEN_INT:
    case TOKEN_DOUBLE:
    case TOKEN_STRING:
        return true;
    default:
        return false;
    }
}

// helper function -> can token be at the start of a block statement?
static bool isFirstBlockStat(currentToken *cur) {
    return (cur->token.type == TOKEN_KEYWORD && (cur->token.data.keywordVal == KEYWORD_IF ||
                                                    cur->token.data.keywordVal == KEYWORD_WHILE));
}

// helper function -> can token be at the start of a statement?
static bool isFirstStat(currentToken *cur) {
    if (isFirstExp(cur)) {
        return true;
    }

    if (cur->token.type == TOKEN_KEYWORD) {
        switch (cur->token.data.keywordVal) {
        case KEYWORD_VAL:
        case KEYWORD_VAR:
        case KEYWORD_RETURN:
            return true;
        default:
            return false;
        }
    }
    return false;
}

bool parseType(currentToken *cur, dataType *outType) {
    if (cur->token.type == TOKEN_KEYWORD) {
        switch (cur->token.data.keywordVal) {
        case KEYWORD_UNIT:
            *outType = TYPE_UNIT;
            return nextToken(cur);
        case KEYWORD_STRING:
            *outType = TYPE_STRING;
            return nextToken(cur);
        case KEYWORD_INT:
            *outType = TYPE_INT;
            return nextToken(cur);
        case KEYWORD_DOUBLE:
            *outType = TYPE_DOUBLE;
            return nextToken(cur);
        default:
            break;
        }
    }

    errorParser(cur->token.line, "Syntax error: expected data type");
    return false;
}

bool parseRetVal(currentToken *cur, ASTNode **outExpr) {
    // return can be empty and have eol or ';' after it
    if (cur->token.type == TOKEN_SEMICOLON || cur->token.type == TOKEN_EOL) {
        *outExpr = NULL;
        return true;
    }

    // parse return expression
    if (isFirstExp(cur)) {
        return parseExpression(cur, NULL, outExpr);
    }

    errorParser(
        cur->token.line, "Syntax error: expected return expression, ';', or EOL after 'return'");
    return false;
}

static bool parseWhile(currentToken *cur, ASTNode **outStmt) {
    if (!nextToken(cur)) {
        return false;
    }

    ASTNode *whileNode = createNode(NODE_WHILE);
    if (whileNode == NULL) {
        errorInternal(cur->token.line, "Memory allocation failed for NODE_WHILE");
        return false;
    }
    *outStmt = whileNode;

    // parse condition
    if (!parseExpression(cur, NULL, &whileNode->data.whileStmt.condition)) {
        return false;
    }

    if (!expectKeyword(cur, KEYWORD_DO, "Syntax error: missing keyword 'do' after condition")) {
        return false;
    }
    if (!expectToken(cur, TOKEN_EOL, "Syntax error: missing EOL after 'do'")) {
        return false;
    }

    // parse do block
    return parseBlock(cur, &whileNode->data.whileStmt.bodyBlock);
}

static bool parseIf(currentToken *cur, ASTNode **outStmt) {
    if (!nextToken(cur)) {
        return false;
    }

    ASTNode *ifNode = createNode(NODE_IF);
    if (ifNode == NULL) {
        errorInternal(cur->token.line, "Memory allocation failed for NODE_IF");
        return false;
    }
    *outStmt = ifNode;

    // parse condition
    if (!parseExpression(cur, NULL, &ifNode->data.ifStmt.condition)) {
        return false;
    }

    if (!expectKeyword(cur, KEYWORD_THEN, "Syntax error: missing keyword 'then' after condition")) {
        return false;
    }
    if (!expectToken(cur, TOKEN_EOL, "Syntax error: missing EOL after 'then'")) {
        return false;
    }

    // parse then block
    if (!parseBlock(cur, &ifNode->data.ifStmt.thenBlock)) {
        return false;
    }

    if (!expectKeyword(cur, KEYWORD_ELSE, "Syntax error: missing mandatory 'else' branch")) {
        return false;
    }
    if (!expectToken(cur, TOKEN_EOL, "Syntax error: missing EOL after 'else'")) {
        return false;
    }

    // parse else block
    return parseBlock(cur, &ifNode->data.ifStmt.elseBlock);
}

bool parseBlockStat(currentToken *cur, ASTNode **outStmt) {
    // parse if
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_IF) {
        return parseIf(cur, outStmt);
    }

    // parse while
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_WHILE) {
        return parseWhile(cur, outStmt);
    }

    errorParser(cur->token.line, "Syntax error: expected 'if' or 'while' statement");
    return false;
}

static bool parseVariableDec(currentToken *cur, bool isMutable, ASTNode **outStmt) {
    if (cur->token.type != TOKEN_IDENTIFIER) {
        errorParser(cur->token.line, "Syntax error: missing variable identifier");
        return false;
    }

    ASTNode *declNode = createNode(NODE_VAR_DECL);
    if (declNode == NULL) {
        errorInternal(cur->token.line, "Memory allocation failed for NODE_VAR_DECL");
        return false;
    }
    declNode->data.varDecl.name = cur->token.data.identifier;
    declNode->data.varDecl.isMutable = isMutable;
    declNode->data.varDecl.varType = TYPE_UNKNOWN;
    *outStmt = declNode;

    if (!nextToken(cur)) {
        return false;
    }

    if (!parseOptType(cur, &declNode->data.varDecl.varType)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_EQUAL_SIGN, "Syntax error: missing '=' before initialization")) {
        return false;
    }

    return parseExpression(cur, NULL, &declNode->data.varDecl.initExpr);
}

bool parseStat(currentToken *cur, ASTNode **outStmt) {
    // parse val declaration (immutable)
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_VAL) {
        return (nextToken(cur) && parseVariableDec(cur, false, outStmt));
    }

    // parse var declaration (mutable)
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_VAR) {
        return (nextToken(cur) && parseVariableDec(cur, true, outStmt));
    }

    // parse return
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_RETURN) {
        if (!nextToken(cur)) {
            return false;
        }
        ASTNode *retNode = createNode(NODE_RETURN);
        if (retNode == NULL) {
            errorInternal(cur->token.line, "Memory allocation failed for NODE_RETURN");
            return false;
        }
        *outStmt = retNode;
        return parseRetVal(cur, &retNode->data.returnStmt.expr);
    }

    // parse assignment or expression starting with identifier
    if (cur->token.type == TOKEN_IDENTIFIER) {
        return parseStAfterId(cur, outStmt);
    }

    if (isFirstExp(cur)) {
        return parseExpression(cur, NULL, outStmt);
    }

    errorParser(cur->token.line, "Syntax error: expected 'val', 'var', 'return', or an expression");
    return false;
}

bool parseStRest(currentToken *cur, ASTNode **outNextStmt) {
    // got dedent after statement
    if (cur->token.type == TOKEN_DEDENT) {
        *outNextStmt = NULL;
        return true;
    }

    // parse another statement
    if (isFirstStat(cur)) {
        return (parseStat(cur, outNextStmt) && parseStNext(cur, &(*outNextStmt)->next));
    }

    // parse another block statement
    if (isFirstBlockStat(cur)) {
        return (parseBlockStat(cur, outNextStmt) && parseStRest(cur, &(*outNextStmt)->next));
    }

    errorParser(cur->token.line, "Syntax error: expected next statement or block dedent");
    return false;
}

bool parseStNext(currentToken *cur, ASTNode **outNextStmt) {
    // statement ends with EOL
    if (cur->token.type == TOKEN_EOL) {
        return (nextToken(cur) && parseStRest(cur, outNextStmt));
    }

    // statement ends with semicolon
    if (cur->token.type == TOKEN_SEMICOLON) {
        if (!nextToken(cur)) {
            return false;
        }

        if (!parseOptEol(cur)) {
            return false;
        }

        return parseStRest(cur, outNextStmt);
    }

    errorParser(cur->token.line, "Syntax error: missing EOL or ';' after statement");
    return false;
}

bool parseStList(currentToken *cur, ASTNode **outStmt) {
    // parse normal statements
    if (isFirstStat(cur)) {
        return (parseStat(cur, outStmt) && parseStNext(cur, &(*outStmt)->next));
    }

    // parse block statements (loops and conditions)
    if (isFirstBlockStat(cur)) {
        return (parseBlockStat(cur, outStmt) && parseStRest(cur, &(*outStmt)->next));
    }

    errorParser(cur->token.line, "Syntax error: block must contain at least one valid statement");
    return false;
}

bool parseBlock(currentToken *cur, ASTNode **outBlock) {
    if (!expectToken(
            cur, TOKEN_INDENT, "Syntax error: expected indentation at the start of block")) {
        return false;
    }

    ASTNode *blockNode = createNode(NODE_BLOCK);
    if (blockNode == NULL) {
        errorInternal(cur->token.line, "Memory allocation failed for NODE_BLOCK");
        return false;
    }
    *outBlock = blockNode;

    // parse all the statements in the block
    if (!parseStList(cur, &blockNode->data.block.firstStmt)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_DEDENT, "Syntax error: expected unindent at the end of block")) {
        return false;
    }

    return true;
}

bool parseFooBody(currentToken *cur, ASTNode **outBody) {
    // parse function with a block body
    if (cur->token.type == TOKEN_EOL) {
        return (nextToken(cur) && parseBlock(cur, outBody));
    }

    // parse one line function
    if (isFirstExp(cur)) {
        if (!parseExpression(cur, NULL, outBody)) {
            return false;
        }
        return expectToken(
            cur, TOKEN_EOL, "Syntax error: missing EOL after single-line function expression");
    }

    errorParser(
        cur->token.line, "Syntax error: expected EOL (for indented block) or expression after '='");
    return false;
}

bool parseOptType(currentToken *cur, dataType *outType) {
    // there is a type
    if (cur->token.type == TOKEN_COLON) {
        return (nextToken(cur) && parseType(cur, outType));
    }

    // there is not a type
    if (cur->token.type == TOKEN_EQUAL_SIGN) {
        *outType = TYPE_UNKNOWN;
        return true;
    }

    errorParser(cur->token.line, "Syntax error: expected ':' (for type annotation) or '='");
    return false;
}

static bool parseSingleParam(currentToken *cur, ASTNode **outParam) {
    ASTNode *paramNode = createNode(NODE_PARAM);
    if (paramNode == NULL) {
        errorInternal(cur->token.line, "Memory allocation failed for NODE_PARAM");
        return false;
    }
    paramNode->data.param.name = cur->token.data.identifier;
    *outParam = paramNode;

    if (!nextToken(cur)) {
        return false;
    }

    if (!parseOptEol(cur)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_COLON, "Syntax error: missing ':' after parameter name")) {
        return false;
    }

    if (!parseOptEol(cur)) {
        return false;
    }
    if (!parseType(cur, &paramNode->data.param.type)) {
        return false;
    }
    if (!parseOptEol(cur)) {
        return false;
    }

    return parseNextParam(cur, &paramNode->next);
}

bool parseAfterCom(currentToken *cur, ASTNode **outParam) {
    // parse next parameter
    if (cur->token.type == TOKEN_IDENTIFIER) {
        return parseSingleParam(cur, outParam);
    }

    // parse EOL in parameters
    if (cur->token.type == TOKEN_EOL) {
        return (nextToken(cur) && parseParams(cur, outParam));
    }

    errorParser(cur->token.line, "Syntax error: expected parameter identifier or EOL after ','");
    return false;
}

bool parseNextParam(currentToken *cur, ASTNode **outParam) {
    // no more parameters
    if (cur->token.type == TOKEN_RIGHT_BRACKET) {
        *outParam = NULL;
        return true;
    }

    // parse comma
    if (cur->token.type == TOKEN_COMMA) {
        return (nextToken(cur) && parseAfterCom(cur, outParam));
    }

    errorParser(cur->token.line,
        "Syntax error: expected ',' for next parameter or ')' to close parameter list");
    return false;
}

bool parseParams(currentToken *cur, ASTNode **outParam) {
    // parse parameter
    if (cur->token.type == TOKEN_IDENTIFIER) {
        return parseSingleParam(cur, outParam);
    }

    // no more parameters (do NOT consume ')' here; parseFooCommon expects it)
    if (cur->token.type == TOKEN_RIGHT_BRACKET) {
        *outParam = NULL;
        return true;
    }

    errorParser(cur->token.line, "Syntax error: expected parameter name or ')'");
    return false;
}

// helper function -> what tokens can immediately follow a missing end of line
static bool isFollowOptEol(currentToken *cur) {
    switch (cur->token.type) {
    case TOKEN_INT:
    case TOKEN_DOUBLE:
    case TOKEN_STRING:
    case TOKEN_IDENTIFIER:
    case TOKEN_LEFT_BRACKET:
    case TOKEN_RIGHT_BRACKET:
    case TOKEN_COLON:
    case TOKEN_COMMA:
    case TOKEN_MINUS:
    case TOKEN_DEDENT:
        return true;
    case TOKEN_KEYWORD:
        switch (cur->token.data.keywordVal) {
        case KEYWORD_DEF:
        case KEYWORD_VAL:
        case KEYWORD_VAR:
        case KEYWORD_RETURN:
        case KEYWORD_IF:
        case KEYWORD_WHILE:
        case KEYWORD_INT:
        case KEYWORD_DOUBLE:
        case KEYWORD_STRING:
        case KEYWORD_UNIT:
            return true;
        default:
            return false;
        }
    default:
        return false;
    }
}

bool parseOptEol(currentToken *cur) {
    // there is an EOL
    if (cur->token.type == TOKEN_EOL) {
        return nextToken(cur);
    }

    // there is not an EOL
    if (isFollowOptEol(cur)) {
        return true;
    }

    errorParser(cur->token.line, "Syntax error: unexpected token after optional newline");
    return false;
}

static bool parseFooCommon(currentToken *cur, bool isMain, ASTNode **outFunc) {
    if (cur->token.type != TOKEN_IDENTIFIER) {
        errorParser(cur->token.line, "Syntax error: missing function identifier");
        return false;
    }

    ASTNode *funcNode = createNode(NODE_FUNC_DEF);
    if (funcNode == NULL) {
        errorInternal(cur->token.line, "Memory allocation failed for NODE_FUNC_DEF");
        return false;
    }
    funcNode->data.funcDef.name = cur->token.data.identifier;
    funcNode->data.funcDef.isMain = isMain;
    funcNode->data.funcDef.returnType = TYPE_UNKNOWN;
    *outFunc = funcNode;

    if (!nextToken(cur)) {
        return false;
    }
    if (!expectToken(
            cur, TOKEN_LEFT_BRACKET, "Syntax error: missing '(' after function identifier")) {
        return false;
    }

    if (!parseOptEol(cur)) {
        return false;
    }
    if (!parseParams(cur, &funcNode->data.funcDef.params)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_RIGHT_BRACKET, "Syntax error: missing ')' after parameter list")) {
        return false;
    }

    if (!parseOptType(cur, &funcNode->data.funcDef.returnType)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_EQUAL_SIGN, "Syntax error: missing '=' before function body")) {
        return false;
    }

    return parseFooBody(cur, &funcNode->data.funcDef.body);
}

static bool parseMainFoo(currentToken *cur, ASTNode **outFunc) {
    if (!parseOptEol(cur)) {
        return false;
    }

    if (!expectKeyword(
            cur, KEYWORD_DEF, "Syntax error: missing keyword 'def' in main function declaration")) {
        return false;
    }

    return parseFooCommon(cur, true, outFunc);
}

static bool parseFoo(currentToken *cur, ASTNode **outFunc) {
    return parseFooCommon(cur, false, outFunc);
}

bool parseFooDec(currentToken *cur, ASTNode **outFunc) {
    // parsing regular function
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_DEF) {
        return (nextToken(cur) && parseFoo(cur, outFunc));
    }

    // parsing main function
    if (cur->token.type == TOKEN_MAIN) {
        return (nextToken(cur) && parseMainFoo(cur, outFunc));
    }

    errorParser(cur->token.line, "Syntax error: expected 'def' or '@main' in function declaration");
    return false;
}

bool parseFooList(currentToken *cur, ASTNode **outFuncList) {
    // parsing function
    if ((cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_DEF) ||
        (cur->token.type == TOKEN_MAIN)) {
        return (parseFooDec(cur, outFuncList) && parseFooList(cur, &(*outFuncList)->next));
    }

    // no more functions
    if (cur->token.type == TOKEN_EOF) {
        *outFuncList = NULL;
        return true;
    }

    errorParser(cur->token.line, "Syntax error: expected function declaration or EOF");
    return false;
}

bool parseProg(currentToken *cur, ASTNode **outProgram) {
    *outProgram = NULL;

    if (!nextToken(cur)) {
        return false;
    }
    if (!expectKeyword(cur, KEYWORD_IMPORT, "Syntax error: missing keyword 'import' in prolog")) {
        return false;
    }

    if (cur->token.type != TOKEN_IDENTIFIER || strcmp(cur->token.data.identifier, "ifj26") != 0) {
        errorParser(cur->token.line,
            "Syntax error: expected package name 'ifj26' after 'import' in prolog");
        return false;
    }
    // Free prolog package identifier since it is not stored in the AST
    free(cur->token.data.identifier);
    cur->token.data.identifier = NULL;

    if (!nextToken(cur)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_PERIOD, "Syntax error: missing '.' after 'ifj26' in prolog")) {
        return false;
    }
    if (!expectToken(cur, TOKEN_MULTIPLY, "Syntax error: missing '*' after 'ifj26.' in prolog")) {
        return false;
    }
    if (!expectToken(
            cur, TOKEN_EOL, "Syntax error: missing EOL after 'import ifj26.*' in prolog")) {
        return false;
    }

    ASTNode *progNode = createTree();
    if (progNode == NULL) {
        errorInternal(cur->token.line, "Memory allocation failed for NODE_PROGRAM");
        return false;
    }

    // parse all the functions in the program
    if (!parseFooList(cur, &progNode->data.program.functions)) {
        deleteNode(progNode);
        return false;
    }

    // Last token
    if (cur->token.type != TOKEN_EOF) {
        errorParser(cur->token.line, "Syntax error: expected EOF at the end of program");
        deleteNode(progNode);
        return false;
    }

    size_t count = 0;
    for (ASTNode *fn = progNode->data.program.functions; fn != NULL; fn = fn->next) {
        count++;
    }
    progNode->data.program.funcCount = count;

    *outProgram = progNode;
    return true;
}
