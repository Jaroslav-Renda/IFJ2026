/**
 * @file    ast.h
 * @brief   Source file for abstract syntax tree
 * @author  Jakub Poláček (xpolacj00)
 * @date    2026-11-12
 * @details project: IFJ2026
 */
#ifndef AST_H
#define AST_H

#include "symtable.h"
#include "symtableStack.h"
#include <stdbool.h>
#include <stdint.h>

// All types of nodes all these nodes have union in NodeType
typedef enum : uint8_t {
    NODE_PROGRAM, // The root of the tree only points to the first function next
                  // are accesible through next* in the function, also holds
                  // count of all functions
    NODE_BLOCK,   // Represents block of code

    // Definitions Statements
    NODE_FUNC_DEF, // Definition of a function
    NODE_PARAM,    // Parameters in funciton header
    NODE_VAR_DECL, // Deklaration of variable
    NODE_ASSIGN,   // Assigning to variable
    NODE_IF,       // Condition if (cond) { thenBlock } else { elseBlock }
    NODE_WHILE,    // Cycle while (cond) { bodyBlock }
    NODE_RETURN,   // Return expr

    // Expressions
    NODE_BIN_OP,    // Binary opreators: +, -, *, /, ==, !=, <, > ...
    NODE_UNARY_OP,  // Unary operators -x or !x)
    NODE_FUNC_CALL, // Calling a function
    NODE_VAR_REF,   // Variable Reference
    NODE_LITERAL    // const Literal
} ASTNodeType;

// types of binary operators
typedef enum : uint8_t {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_EQ,
    OP_NEQ,
    OP_LT,  // <
    OP_GT,  // >
    OP_LTE, // <=
    OP_GTE  // >=
} BinaryOpType;

// types of unary operators
typedef enum : uint8_t {
    UOP_MINUS, // -x
    UOP_PLUS   // +x
} UnaryOpType;

typedef struct ASTNode ASTNode;

struct ASTNode {
    ASTNodeType type; // type of Nodes stated upwards
    ASTNode *next;    // points to next Node can be: next function, next command in
                      // a block, next argument, next parameter
    // semantic information to be filled during semantical analysis
    dataType semanticType; // Final data type of node
    symtableItem *symbol;  // pointer to symtable to the specific symbol
    bool convertToDouble;  // for code generetor when it should call int->float
    // Specific data for each node type
    union {
        struct {
            ASTNode *functions; // points to first function in program
            size_t funcCount;   // count of functions in program
        } program;

        struct {
            ASTNode *firstStmt; // First statement in block
        } block;

        struct {
            char *name;
            dataType returnType;
            ASTNode *params; // points to first param next are accesible through
                             // next*
            ASTNode *body;   // points to first statement in function body
        } funcDef;

        struct {
            char *name;
            dataType type;
        } param;

        struct {
            char *name;        // "a"
            dataType varType;  // UNKNOWN if not stated
            bool isMutable;    // true for var, false for val
            ASTNode *initExpr; // pointer to root of expresion
        } varDecl;

        struct {
            char *targetName;
            ASTNode *expr; // Right hand side expresion
        } assign;

        struct {
            ASTNode *condition; // condition expresion
            ASTNode *thenBlock; // pointer to first statement in then block
            ASTNode *elseBlock; // pointer to first statement in else block
        } ifStmt;

        struct {
            ASTNode *condition; // condition expresion
            ASTNode *bodyBlock; // points to first statement of the block
        } whileStmt;

        struct {
            ASTNode *expr; // points to returned expresion
        } returnStmt;

        struct {
            BinaryOpType op; // type of operand
            ASTNode *left;   // left subexpresion
            ASTNode *right;  // right subexpresion
        } binOp;

        struct {
            char *funcName;
            ASTNode *args; // points to first argument next are accesible
                           // through next*
        } funcCall;

        struct {
            UnaryOpType op;
            ASTNode *operand;
        } unaryOp;

        struct {
            char *name;
        } varRef;

        struct {
            dataType litType;
            union {
                int64_t intVal;
                double doubleVal;
                char *strVal;
            } value;
        } literal;
    } data;
};

typedef struct {
    Symtable *globalFuncTable; // table of all functions
    stackPtr localScopeStack;  // pointer to the top of stack of scopes
    symtableItem *currentFunc; // currently checked function
} SemanticContext;

void destroyTree(ASTNode *tree);

ASTNode *createTree();

ASTNode *addNode(ASTNodeType type, ASTNode *parent);

void deleteNode(ASTNodeType type, ASTNode *node);

#endif // AST_H
