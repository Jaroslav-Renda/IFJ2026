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
    NODE_PROGRAM,
    /**
     *The root of the tree only points to the first function next are accesible
     * through next* in the function, also holds count of all functions
     */
    NODE_BLOCK, /// Represents block of code

    /// Definitions Statements
    NODE_FUNC_DEF, /// Definition of a function
    NODE_PARAM,    /// Parameters in funciton header
    NODE_VAR_DECL, /// Deklaration of variable
    NODE_ASSIGN,   /// Assigning to variable
    NODE_IF,       /// Condition if (cond) { thenBlock } else { elseBlock }
    NODE_WHILE,    /// Cycle while (cond) { bodyBlock }
    NODE_RETURN,   /// Return expr

    /// Expressions
    NODE_BIN_OP,    /// Binary opreators: +, -, *, /, ==, !=, <, > ...
    NODE_UNARY_OP,  /// Unary operators -x or !x)
    NODE_FUNC_CALL, /// Calling a function
    NODE_VAR_REF,   /// Variable Reference
    NODE_LITERAL,   /// const Literal
} ASTNodeType;

/// types of binary operators
typedef enum : uint8_t {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_EQ,
    OP_NEQ,
    OP_LT,  /// <
    OP_GT,  /// >
    OP_LTE, /// <=
    OP_GTE, /// >=
} BinaryOpType;

/// types of unary operators
typedef enum : uint8_t {
    UOP_MINUS, /// -x
    UOP_PLUS,  /// +x
} UnaryOpType;

typedef struct FuncVarEntry {
    char *origName;            /**< Original identifier name in source code. */
    char *mangledName;         /**< Unique mangled name for code generation (e.g. "foo$x$1"). */
    dataType type;             /**< Resolved/inferred data type of the variable. */
    bool isMutable;            /**< True for `var`, false for `val` or parameter. */
    struct FuncVarEntry *next; /**< Next variable in the function's variable list. */
} FuncVarEntry;

typedef struct ASTNode ASTNode;

struct ASTNode {
    ASTNodeType type; /// type of Nodes stated upwards
    ASTNode *next;    /// points to next Node can be: next function, next command in
                      /// a block, next argument, next parameter
    /// semantic information to be filled during semantical analysis
    dataType semanticType; /// Final data type of node
    bool convertToDouble;  /// for code generetor when it should call int->float
    /// Specific data for each node type
    union { /// data
        /**
         * @brief structure for data of root node program
         */
        struct {
            /// points to first function in program
            ASTNode *functions;
            /// count of functions in program
            size_t funcCount;
        } program;

        /**
         * @brief structure for data of node for block
         */
        struct {
            /// First statement in block
            ASTNode *firstStmt;
        } block;

        /**
         * @brief structure for data of node for definition of a function
         */
        struct {
            char *name;
            char *mangledName;
            dataType returnType;
            /// points to first param next are accesible through next*
            ASTNode *params;
            /// points to first statement in function body
            ASTNode *body;
            /// Linked list of all variables belonging to this function.
            FuncVarEntry *vars;
            /// number of variables in function
            size_t varCount;
            bool isMain;
        } funcDef;

        /**
         * @brief structure for data of node for parameters of function
         */
        struct {
            char *name;
            dataType type;
        } param;

        /**
         * @brief structure for data of node for declaring a variable
         */
        struct {
            char *name;
            char *mangledName;
            /// UNKNOWN if not stated
            dataType varType;
            /// true for var, false for val
            bool isMutable;
            /// pointer to root of expresion
            ASTNode *initExpr;
        } varDecl;

        /**
         * @brief structure for data of node for assigning to a variable
         */
        struct {
            char *targetName;
            char *mangledName;
            ASTNode *expr; /// Right hand side expresion
        } assign;

        /**
         * @brief structure for data of node for if statement
         */
        struct {
            /// unique index for code generator shared for all functions
            size_t idx;
            /// points to root of condition expresion
            ASTNode *condition;
            /// pointer to first statement in then block
            ASTNode *thenBlock;
            /// pointer to first statement in else block
            ASTNode *elseBlock;
        } ifStmt;

        /**
         * @brief structure for data of node for while statement
         */
        struct {
            /// unique index for code generator shared for all functions
            size_t idx;
            /// condition expresion
            ASTNode *condition;
            /// points to first statement of the block
            ASTNode *bodyBlock;
        } whileStmt;

        /**
         * @brief structure for data of node for return statement
         */
        struct {
            /// points to returned expresion
            ASTNode *expr;
        } returnStmt;

        /**
         * @brief structure for data of node for binary operator
         */
        struct {
            /// type of operator
            BinaryOpType op;
            /// left subexpresion/// points to first argument next are accesible through next*
            ASTNode *left;
            /// right subexpresion
            ASTNode *right;
        } binOp;

        /**
         * @brief structure for data of node for function call
         */
        struct {
            char *funcName;
            char *mangledName;
            /// points to first argument next are accesible through next*
            ASTNode *args;
        } funcCall;

        /**
         * @brief structure for data of node for unary operator
         */
        struct {
            UnaryOpType op;
            ASTNode *operand;
        } unaryOp;

        /**
         * @brief structure for data of node for reference of variable/value
         */
        struct {
            char *name;
            char *mangledName;
        } varRef;

        /**
         * @brief structure for data of node for literal
         */
        struct {
            /// int double string
            dataType litType;
            union {
                int64_t intVal;
                double doubleVal;
                char *strVal;
            } value;
        } literal;
    } data;
};

/**
 * @brief structure that is sent to semantic checks holds pointer to table with all functions of our
 * program, pointer to stack of scopes and poiter to function that is curently being examined
 */
typedef struct {
    /// table of all functions
    Symtable *globalFuncTable;
    /// pointer to the top of stack of scopes
    stackPtr localScopeStack;
    /// currently checked function
    symtableItem *currentFunc;
    /// id of next variable reseted for each function
    size_t nextVarId;
    /// pointer to the end of linked list of variables of each function
    FuncVarEntry *varListTail;
    /// counters for globally unique `if`/`while` labels shared between functions
    size_t nextwhileLabelId;
    size_t nextIfLabelId;
} SemanticContext;

/**
 * @brief Creates program node the root node
 * @return Pointer to the program node
 */
ASTNode *createTree();

/**
 * @brief creates and allocs memory for new node
 * @param type type of node you want to be created
 * @return pointer to newly created node
 */
ASTNode *createNode(ASTNodeType type);

/**
 * @brief deletes node and everything that lies after it in the tree
 * @param node* points to the node you want to delete
 */
void deleteNode(ASTNode *node);

#endif // AST_H
