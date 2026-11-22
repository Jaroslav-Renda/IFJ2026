/**
 * @file    parser.c
 * @brief   Implementation of recursive descend syntax parsing
 * @author  Jaroslav Renda (xrendaj00)
 * @details project_IFJ2026
 */

#include "parser.h"
#include "ast.h"
#include "error.h"
#include "lexer.h"
#include "precedence.h"
#include "symtable.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// helper function for getting next token with lexer error handeling
static bool nextToken(currentToken *cur) { return (getToken(&cur->token) == OK); }

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

bool parseStAfterLPar(currentToken *cur, ASTNode **statement) {
    lookaheadToken lat;
    lat.token = cur->token;
    lat.isEmpty = false;
    if (!nextToken(cur)) {
        return false;
    }

    // is ()
    if (cur->token.type == TOKEN_RIGHT_BRACKET) {
        lat.isEmpty = true;
        if (!nextToken(cur)) {
            return false;
        }

        ASTNode *node = createNode(NODE_LITERAL);
        if (node == NULL) {
            errorInternal(cur->token.line, "mem allocation failed");
            return false;
        }
        node->data.literal.litType = TYPE_UNIT;
        *statement = node;
        return true;
    }

    // not () -> expression (E)
    if (!parseExpression(cur, &lat)) {
        return false;
    }
    return expectToken(cur, TOKEN_EOL, "expected EOL after expression");
}

bool parseStAfterId(currentToken *cur, ASTNode **statement) {
    // lookahead token holds id
    lookaheadToken lat;
    lat.token = cur->token;
    lat.isEmpty = true;
    if (!nextToken(cur)) {
        free(lat.token.data.identifier);
        return false;
    }

    // assignment
    if (cur->token.type == TOKEN_EQUAL_SIGN) {
        if (!nextToken(cur)) {
            free(lat.token.data.identifier);
            return false;
        }

        ASTNode *tmpAssignNode = createNode(NODE_ASSIGN);
        if (tmpAssignNode == NULL) {
            free(lat.token.data.identifier);
            errorInternal(cur->token.line, "mem allocation for NODE_ASSIGN failed");
            return false;
        }
        tmpAssignNode->data.assign.targetName = lat.token.data.identifier;
        *statement = tmpAssignNode;

        // TODO: ast in precedence
        return parseExpression(cur, NULL);
    }

    if (isLookaheadExpressionFirst(cur)) {
        lat.isEmpty = false;
        // TODO: ast in precedence
        return parseExpression(cur, &lat);
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

bool parseType(currentToken *cur, dataType *type) {
    if (cur->token.type == TOKEN_KEYWORD) {
        switch (cur->token.data.keywordVal) {
        case KEYWORD_UNIT:
            *type = TYPE_UNIT;
            return nextToken(cur);
        case KEYWORD_STRING:
            *type = TYPE_STRING;
            return nextToken(cur);
        case KEYWORD_INT:
            *type = TYPE_INT;
            return nextToken(cur);
        case KEYWORD_DOUBLE:
            *type = TYPE_DOUBLE;
            return nextToken(cur);
        default:
            break;
        }
    }

    errorParser(cur->token.line, "Syntax error: expected data type");
    return false;
}

bool parseRetVal(currentToken *cur, ASTNode **expr) {
    // return can be empty and have eol or ';' after it
    if (cur->token.type == TOKEN_SEMICOLON || cur->token.type == TOKEN_EOL) {
        *expr = NULL;
        return true;
    }

    // parse return expressioin
    if (isFirstExp(cur)) {
        // TODO: ast in precedence
        return parseExpression(cur, NULL);
    }

    errorParser(
        cur->token.line, "Syntax error: expected return expression, ';', or EOL after 'return'");
    return false;
}

static bool parseWhile(currentToken *cur, ASTNode **statement) {
    if (!nextToken(cur)) {
        return false;
    };

    ASTNode *tmpWhile = createNode(NODE_WHILE);
    if (tmpWhile == NULL) {
        errorInternal(cur->token.line, "mem allocation for NODE_WHILE failed");
        return false;
    }
    tmpWhile->data.whileStmt.condition = NULL;
    tmpWhile->data.whileStmt.bodyBlock = NULL;
    *statement = tmpWhile;

    // parse condition
    // TODO: ast in precedence
    if (!parseExpression(cur, NULL)) {
        return false;
    }

    if (!expectKeyword(cur, KEYWORD_DO, "Syntax error: missing keyword 'do' after condition")) {
        return false;
    }
    if (!expectToken(cur, TOKEN_EOL, "Syntax error: missing EOL after 'do'")) {
        return false;
    }

    // parse do block
    if (!parseBlock(cur, &tmpWhile->data.whileStmt.bodyBlock)) {
        return false;
    }

    return true;
}

static bool parseIf(currentToken *cur, ASTNode **statement) {
    if (!nextToken(cur)) {
        return false;
    };

    ASTNode *tmpIfNode = createNode(NODE_IF);
    if (tmpIfNode == NULL) {
        errorInternal(cur->token.line, "mem allocation for NODE_IF failed");
        return false;
    }
    tmpIfNode->data.ifStmt.condition = NULL;
    tmpIfNode->data.ifStmt.thenBlock = NULL;
    tmpIfNode->data.ifStmt.elseBlock = NULL;
    *statement = tmpIfNode;

    // parse condition
    // TODO: ast in precedence
    if (!parseExpression(cur, NULL)) {
        return false;
    }

    if (!expectKeyword(cur, KEYWORD_THEN, "Syntax error: missing keyword 'then' after condition")) {
        return false;
    }
    if (!expectToken(cur, TOKEN_EOL, "Syntax error: missing EOL after 'then'")) {
        return false;
    }

    // parse if block
    if (!parseBlock(cur, &tmpIfNode->data.ifStmt.thenBlock)) {
        return false;
    }

    if (!expectKeyword(cur, KEYWORD_ELSE, "Syntax error: missing mandatory 'else' branch")) {
        return false;
    }
    if (!expectToken(cur, TOKEN_EOL, "Syntax error: missing EOL after 'else'")) {
        return false;
    }

    // parse else block
    if (!parseBlock(cur, &tmpIfNode->data.ifStmt.elseBlock)) {
        return false;
    }

    return true;
}

bool parseBlockStat(currentToken *cur, ASTNode **statement) {
    // parse if
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_IF) {
        return parseIf(cur, statement);
    }

    // parse while
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_WHILE) {
        return parseWhile(cur, statement);
    }

    errorParser(cur->token.line, "Syntax error: expected 'if' or 'while' statement");
    return false;
}

static bool parseVariableDec(currentToken *cur, bool isMutable, ASTNode **statement) {
    if (cur->token.type != TOKEN_IDENTIFIER) {
        errorParser(cur->token.line, "Syntax error: missing variable identifier");
        return false;
    }

    ASTNode *tmpVarNode = createNode(NODE_VAR_DECL);
    if (tmpVarNode == NULL) {
        errorInternal(cur->token.line, "mem allocation for NODE_VAR_DECL failed");
        return false;
    }
    tmpVarNode->data.varDecl.name = cur->token.data.identifier;
    tmpVarNode->data.varDecl.isMutable = isMutable;
    tmpVarNode->data.varDecl.varType = TYPE_UNKNOWN;
    *statement = tmpVarNode;

    if (!nextToken(cur)) {
        return false;
    }

    if (!parseOptType(cur, &tmpVarNode->data.varDecl.varType)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_EQUAL_SIGN, "Syntax error: missing '=' before initialization")) {
        return false;
    }

    // TODO: ast in precedence
    return parseExpression(cur, NULL);
}

bool parseStat(currentToken *cur, ASTNode **statement) {
    // parse val declaration
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_VAL) {
        return (nextToken(cur) && parseVariableDec(cur, false, statement));
    }

    // parse var declaration
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_VAR) {
        return (nextToken(cur) && parseVariableDec(cur, true, statement));
    }

    // parse return
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_RETURN) {
        ASTNode *tmpReturnNode = createNode(NODE_RETURN);
        if (tmpReturnNode == NULL) {
            errorInternal(cur->token.line, "mem allocation for NODE_RETURN failed");
            return false;
        }
        *statement = tmpReturnNode;

        return (nextToken(cur) && parseRetVal(cur, &tmpReturnNode->data.returnStmt.expr));
    }

    // parse expression
    if (cur->token.type == TOKEN_IDENTIFIER) {
        return parseStAfterId(cur, statement);
    }

    if (cur->token.type == TOKEN_LEFT_BRACKET) {
        return parseStAfterLPar(cur, statement);
    }

    if (isFirstExp(cur)) {
        // TODO: ast in precedence
        return parseExpression(cur, NULL);
    }

    errorParser(cur->token.line, "Syntax error: expected 'val', 'var', 'return', or an expression");
    return false;
}

bool parseStRest(currentToken *cur, ASTNode **nextStatement) {
    // got indentation after statement
    if (cur->token.type == TOKEN_DEDENT) {
        return true;
    }

    // parse another statement
    if (isFirstStat(cur)) {
        return (parseStat(cur, nextStatement) && parseStNext(cur, &(*nextStatement)->next));
    }

    // parse another block statement
    if (isFirstBlockStat(cur)) {
        return (parseBlockStat(cur, nextStatement) && parseStRest(cur, &(*nextStatement)->next));
    }

    errorParser(cur->token.line, "Syntax error: expected next statement or block dedent");
    return false;
}

bool parseStNext(currentToken *cur, ASTNode **nextStatement) {
    // statement ends with EOL
    if (cur->token.type == TOKEN_EOL) {
        // parse another statement after a EOL
        return (nextToken(cur) && parseStRest(cur, nextStatement));
    }

    // statement ends with semicolon
    if (cur->token.type == TOKEN_SEMICOLON) {
        if (!nextToken(cur)) {
            return false;
        }

        if (!parseOptEol(cur)) {
            return false;
        }

        // parse another statement after a ';'
        return parseStRest(cur, nextStatement);
    }

    errorParser(cur->token.line, "Syntax error: missing EOL or ';' after statement");
    return false;
}

bool parseStList(currentToken *cur, ASTNode **statement) {
    // parse normal statements
    if (isFirstStat(cur)) {
        return (parseStat(cur, statement) && parseStNext(cur, &(*statement)->next));
    }

    // parse block statements (loops and conditions)
    if (isFirstBlockStat(cur)) {
        return (parseBlockStat(cur, statement) && parseStRest(cur, &(*statement)->next));
    }

    errorParser(cur->token.line, "Syntax error: block must contain at least one valid statement");
    return false;
}

bool parseBlock(currentToken *cur, ASTNode **block) {
    if (!expectToken(
            cur, TOKEN_INDENT, "Syntax error: expected indentation at the start of block")) {
        return false;
    }

    ASTNode *tmpBlockNode = createNode(NODE_BLOCK);
    if (tmpBlockNode == NULL) {
        errorInternal(cur->token.line, "mem allocation for NODE_BLOCK failed");
        return false;
    }
    *block = tmpBlockNode;
    // parse all the statements in the block
    if (!parseStList(cur, &tmpBlockNode->data.block.firstStmt)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_DEDENT, "Syntax error: expected unindent at the end of block")) {
        return false;
    }

    return true;
}

bool parseFooBody(currentToken *cur, ASTNode **fooBody) {
    // parse function with a block body
    if (cur->token.type == TOKEN_EOL) {
        return (nextToken(cur) && parseBlock(cur, fooBody));
    }

    // parse one line function
    if (isFirstExp(cur)) {
        // TODO: AST in precedence
        if (!parseExpression(cur, NULL)) {
            return false;
        };
        return expectToken(
            cur, TOKEN_EOL, "Syntax error: missing EOL after single-line function expression");
    }

    errorParser(
        cur->token.line, "Syntax error: expected EOL (for indented block) or expression after '='");
    return false;
}

bool parseOptType(currentToken *cur, dataType *type) {
    // there is a type
    if (cur->token.type == TOKEN_COLON) {
        return (nextToken(cur) && parseType(cur, type));
    }

    // there is not a type
    if (cur->token.type == TOKEN_EQUAL_SIGN) {
        *type = TYPE_UNKNOWN;
        return true;
    }

    errorParser(cur->token.line, "Syntax error: expected ':' (for type annotation) or '='");
    return false;
}

bool parseAfterCom(currentToken *cur, ASTNode **nextParam) {
    // parse next parameter
    if (cur->token.type == TOKEN_IDENTIFIER) {
        ASTNode *paramNode = createNode(NODE_PARAM);
        if (paramNode == NULL) {
            errorInternal(cur->token.line, "mem allocation for NODE_PARAM failed");
            return false;
        }
        paramNode->data.param.name = cur->token.data.identifier;
        *nextParam = paramNode;

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

        // parse next parameter
        return parseNextParam(cur, &paramNode->next);
    }

    // parse EOL in parameters
    if (cur->token.type == TOKEN_EOL) {
        return (nextToken(cur) && parseParams(cur, nextParam));
    }

    errorParser(cur->token.line, "Syntax error: expected parameter identifier or EOL after ','");
    return false;
}

bool parseNextParam(currentToken *cur, ASTNode **nextParam) {

    // no more parameters
    if (cur->token.type == TOKEN_RIGHT_BRACKET) {
        *nextParam = NULL;
        return true;
    }

    // parse comma
    if (cur->token.type == TOKEN_COMMA) {
        return (nextToken(cur) && parseAfterCom(cur, nextParam));
    }

    errorParser(cur->token.line,
        "Syntax error: expected ',' for next parameter or ')' to close parameter list");
    return false;
}

bool parseParams(currentToken *cur, ASTNode **fooParams) {
    // parse parameter
    if (cur->token.type == TOKEN_IDENTIFIER) {
        ASTNode *paramNode = createNode(NODE_PARAM);
        if (paramNode == NULL) {
            errorInternal(cur->token.line, "mem allocation for NODE_PARAM failed");
            return false;
        }
        paramNode->data.param.name = cur->token.data.identifier;
        *fooParams = paramNode;

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

        // parse next parameter
        return parseNextParam(cur, &paramNode->next);
    }

    // no more parameters
    if (cur->token.type == TOKEN_RIGHT_BRACKET) {
        *fooParams = NULL;
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

static bool parseMainFoo(currentToken *cur, ASTNode **function) {
    // parse optional eol
    if (!parseOptEol(cur)) {
        return false;
    }

    if (!expectKeyword(
            cur, KEYWORD_DEF, "Syntax error: missing keyword 'def' in main function declaration")) {
        return false;
    }

    if (cur->token.type != TOKEN_IDENTIFIER) {
        errorParser(cur->token.line, "Syntax error: missing function identifier");
        return false;
    }

    ASTNode *tmpFooNode = createNode(NODE_FUNC_DEF);
    if (tmpFooNode == NULL) {
        errorInternal(cur->token.line, "mem allocation for NODE_FUNC_DEF failed");
        return false;
    }
    tmpFooNode->data.funcDef.isMain = true;
    tmpFooNode->data.funcDef.name = cur->token.data.identifier;
    tmpFooNode->data.funcDef.returnType = TYPE_UNKNOWN;
    *function = tmpFooNode;

    if (!nextToken(cur)) {
        return false;
    }

    if (!expectToken(
            cur, TOKEN_LEFT_BRACKET, "Syntax error: missing '(' after function identifier")) {
        return false;
    }

    // parse optional eol
    if (!parseOptEol(cur)) {
        return false;
    }
    // parse function parameters
    if (!parseParams(cur, &tmpFooNode->data.funcDef.params)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_RIGHT_BRACKET, "Syntax error: missing ')' after parameter list")) {
        return false;
    }

    // parse optional return type assignment
    if (!parseOptType(cur, &tmpFooNode->data.funcDef.returnType)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_EQUAL_SIGN, "Syntax error: missing '=' before function body")) {
        return false;
    }

    // parse function body
    if (!parseFooBody(cur, &tmpFooNode->data.funcDef.body)) {
        return false;
    }

    return true;
}

static bool parseFoo(currentToken *cur, ASTNode **function) {
    if (cur->token.type != TOKEN_IDENTIFIER) {
        errorParser(cur->token.line, "Syntax error: missing function identifier");
        return false;
    }

    ASTNode *tmpFooNode = createNode(NODE_FUNC_DEF);
    if (tmpFooNode == NULL) {
        errorInternal(cur->token.line, "mem allocation for NODE_FUNC_DEF failed");
        return false;
    }
    tmpFooNode->data.funcDef.isMain = false;
    tmpFooNode->data.funcDef.name = cur->token.data.identifier;
    tmpFooNode->data.funcDef.returnType = TYPE_UNKNOWN;
    *function = tmpFooNode; // function now points to hte node pointer, so it trackt its changes

    if (!nextToken(cur)) {
        return false;
    }

    if (!expectToken(
            cur, TOKEN_LEFT_BRACKET, "Syntax error: missing '(' after function identifier")) {
        return false;
    }

    // parse optional eol
    if (!parseOptEol(cur)) {
        return false;
    }
    // parse function parameters
    if (!parseParams(cur, &tmpFooNode->data.funcDef.params)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_RIGHT_BRACKET, "Syntax error: missing ')' after parameter list")) {
        return false;
    }

    // parse optional return type assignment
    if (!parseOptType(cur, &tmpFooNode->data.funcDef.returnType)) {
        return false;
    }

    if (!expectToken(cur, TOKEN_EQUAL_SIGN, "Syntax error: missing '=' before function body")) {
        return false;
    }

    // parse function body
    if (!parseFooBody(cur, &tmpFooNode->data.funcDef.body)) {
        return false;
    }

    return true;
}

bool parseFooDec(currentToken *cur, ASTNode **function) {
    // parsing regular function
    if (cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_DEF) {
        return (nextToken(cur) && parseFoo(cur, function));
    }

    // parsing main function
    if (cur->token.type == TOKEN_MAIN) {
        return (nextToken(cur) && parseMainFoo(cur, function));
    }

    errorParser(cur->token.line, "Syntax error: expected 'def' or '@main' in function declaration");
    return false;
}

bool parseFooList(currentToken *cur, ASTNode **function) {
    // parsing function
    if ((cur->token.type == TOKEN_KEYWORD && cur->token.data.keywordVal == KEYWORD_DEF) ||
        (cur->token.type == TOKEN_MAIN)) {
        // parse function declaration and another function
        return (parseFooDec(cur, function) && parseFooList(cur, &(*function)->next));
    }

    // no more functions
    if (cur->token.type == TOKEN_EOF) {
        *function = NULL;
        return true;
    }

    errorParser(cur->token.line, "Syntax error: expected function declaration or EOF");
    return false;
}

bool parseProg(currentToken *cur, ASTNode *program) {
    if (!nextToken(cur)) {
        return false;
    }
    if (!expectKeyword(cur, KEYWORD_IMPORT, "Syntax error: missing keyword 'import' in prolog")) {
        return false;
    }

    // note: strcmp returns 0 when the strings MATCH
    if (cur->token.type != TOKEN_IDENTIFIER || strcmp(cur->token.data.identifier, "ifj26") != 0) {
        errorParser(cur->token.line,
            "Syntax error: expected package name 'ifj26' after 'import' in prolog");
        return false;
    }

    // free the prolog id
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

    // parse all the functions in the program
    if (!parseFooList(cur, &program->data.program.functions)) {
        return false;
    }

    // Last token
    if (cur->token.type != TOKEN_EOF) {
        errorParser(cur->token.line, "Syntax error: expected EOF at the end of program");
        return false;
    }

    // count all program functions
    size_t fooCount = 0;
    for (ASTNode *foo = program->data.program.functions; foo != NULL; foo = foo->next) {
        fooCount++;
    }
    program->data.program.funcCount = fooCount;

    return true;
}
