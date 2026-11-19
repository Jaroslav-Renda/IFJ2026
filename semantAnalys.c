/**
 * @file    semantAnalys.c
 * @brief   Implementation of semantic analysis over the Abstract Syntax Tree (AST)
 * @author  Jakub Poláček (xpolacj00)
 * @date    2026-11-17
 * @details project: IFJ2026
 */

#include "semantAnalys.h"
#include "ast.h"
#include "error.h"
#include "symtable.h"
#include "symtableStack.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Helper structure defining the signature of an IFJ26 built-in function.
 */
typedef struct {
    const char *name;
    size_t paramCount;
    dataType returnType;
    dataType paramTypes[3];
} BuiltinDef;

/**
 * @brief Unwraps a block node to its first statement
 * @param node Pointer to a block wrapper or directly to the first statement node.
 * @return Pointer to the first statement in the sequence, or NULL if empty.
 */
static ASTNode *unwrapBlock(ASTNode *node) {
    if (node != NULL && node->type == NODE_BLOCK) {
        return node->data.block.firstStmt;
    }
    return node;
}

/**
 * @brief Duplicates a C string on the heap
 *
 * @param src Source null-terminated string.
 * @return Heap-allocated copy, or NULL on allocation failure.
 */
static char *dupCString(const char *src) {
    if (src == NULL) {
        return NULL;
    }
    size_t len = strlen(src) + 1;
    char *copy = malloc(len);
    if (copy != NULL) {
        memcpy(copy, src, len);
    }
    return copy;
}

/**
 * @brief Generates a unique mangled name for a variable within a function.
 *
 * @param funcName Enclosing function identifier.
 * @param varName  Original variable identifier.
 * @param varId    Monotonic counter unique within the enclosing function.
 * @return Heap-allocated mangled name string, or NULL on allocation failure.
 */
static char *createMangledVarName(const char *funcName, const char *varName, size_t varId) {
    if (funcName == NULL || varName == NULL) {
        return NULL;
    }
    // Format: "<funcName>$<varName>$<varId>"
    int needed = snprintf(NULL, 0, "%s$%s$%zu", funcName, varName, varId);
    if (needed < 0) {
        return NULL;
    }
    char *buf = malloc((size_t)needed + 1);
    if (buf == NULL) {
        return NULL;
    }
    needed = snprintf(buf, (size_t)needed + 1, "%s$%s$%zu", funcName, varName, varId);
    if (needed < 0 || (size_t)needed >= SIZE_MAX) {
        free(buf);
        return NULL;
    }
    return buf;
}

/**
 * @brief Generates a mangled name for a function (keeps `@main` recognizable or prefixes user
 * funcs).
 *
 * @param funcName Original function name.
 * @param isMain   True if annotated with `@main`.
 * @return Heap-allocated mangled function name, or NULL on failure.
 */
static char *createMangledFuncName(const char *funcName, bool isMain) {
    if (funcName == NULL) {
        return NULL;
    }
    const char *prefix = (int)isMain ? "main$" : "fn$";
    int needed = snprintf(NULL, 0, "%s%s", prefix, funcName);
    if (needed < 0) {
        return NULL;
    }
    char *buf = malloc((size_t)needed + 1);
    if (buf == NULL) {
        return NULL;
    }
    needed = snprintf(buf, (size_t)needed + 1, "%s%s", prefix, funcName);
    if (needed < 0 || (size_t)needed >= SIZE_MAX) {
        free(buf);
        return NULL;
    }
    return buf;
}

/**
 * @brief Appends a declared variable (or parameter) to the enclosing function's variable list.
 *
 * @param ctx         Semantic analysis context (with `currentFunc` set).
 * @param origName    Original variable name.
 * @param mangledName Mangled variable name (ownership transferred to the list entry).
 * @param type        Resolved variable data type.
 * @param isMutable   True if mutable (`var`), false if immutable (`val` or parameter).
 * @return OK on success, ERR_INTERNAL on allocation failure.
 */
static uint8_t appendFuncVar(
    SemanticContext *ctx, const char *origName, char *mangledName, dataType type, bool isMutable) {
    if (ctx == NULL || ctx->currentFunc == NULL || ctx->currentFunc->data.func.node == NULL) {
        free(mangledName);
        return ERR_INTERNAL;
    }

    ASTNode *funcNode = ctx->currentFunc->data.func.node;
    FuncVarEntry *entry = calloc(1, sizeof(FuncVarEntry));
    if (entry == NULL) {
        free(mangledName);
        return ERR_INTERNAL;
    }

    entry->origName = dupCString(origName);
    if (entry->origName == NULL) {
        free(mangledName);
        free(entry);
        return ERR_INTERNAL;
    }

    entry->mangledName = mangledName;
    entry->type = type;
    entry->isMutable = isMutable;
    entry->next = NULL;

    if (funcNode->data.funcDef.vars == NULL) {
        funcNode->data.funcDef.vars = entry;
    } else {
        ctx->varListTail->next = entry;
    }
    ctx->varListTail = entry;
    funcNode->data.funcDef.varCount++;
    return OK;
}

/**
 * @brief Inserts a single built-in function definition into the global function symbol table.
 *
 * @param table Pointer to the global function symbol table.
 * @param def   Pointer to the built-in function descriptor.
 * @return OK on success, ERR_INTERNAL on memory allocation failure.
 */
static uint8_t addSingleBuiltin(Symtable *table, const BuiltinDef *def) {
    symtableItem *item = symtableLookupAdd(table, def->name);
    if (item == NULL) {
        return ERR_INTERNAL;
    }

    item->kind = SYM_FUNC;
    item->data.func.node = NULL;
    item->data.func.isBuiltin = true;
    item->data.func.isMain = false;
    item->data.func.isEvaluated = true;
    item->data.func.beingEvaluated = false;
    item->data.func.hasExplicitReturnType = true;
    item->data.func.hasReturnStmt = false;
    item->data.func.returnType = def->returnType;
    item->data.func.paramCount = def->paramCount;

    if (def->paramCount > 0) {
        item->data.func.params = calloc(def->paramCount, sizeof(*item->data.func.params));
        if (item->data.func.params == NULL) {
            return ERR_INTERNAL;
        }
        for (size_t i = 0; i < def->paramCount; i++) {
            item->data.func.params[i].name = NULL;
            item->data.func.params[i].type = def->paramTypes[i];
        }
    } else {
        item->data.func.params = NULL;
    }

    return OK;
}

/**
 * @brief Pre-populates the global function table with all 10 IFJ26 built-in functions.
 *
 * @param table Pointer to the global function symbol table.
 * @return OK on success, ERR_INTERNAL on allocation error.
 */
static uint8_t registerBuiltins(Symtable *table) {
    static const BuiltinDef builtins[] = {{"print", 1, TYPE_UNIT, {TYPE_ANY}},
        {"readLine", 0, TYPE_STRING, {TYPE_UNKNOWN}}, {"readInt", 0, TYPE_INT, {TYPE_UNKNOWN}},
        {"readDouble", 0, TYPE_DOUBLE, {TYPE_UNKNOWN}}, {"toStr", 1, TYPE_STRING, {TYPE_ANY}},
        {"toInt", 1, TYPE_INT, {TYPE_DOUBLE}}, {"strLength", 1, TYPE_INT, {TYPE_STRING}},
        {"substr", 3, TYPE_STRING, {TYPE_STRING, TYPE_INT, TYPE_INT}},
        {"ord", 2, TYPE_INT, {TYPE_STRING, TYPE_INT}}, {"chr", 1, TYPE_STRING, {TYPE_INT}}};

    size_t count = sizeof(builtins) / sizeof(builtins[0]);
    for (size_t i = 0; i < count; i++) {
        uint8_t err = addSingleBuiltin(table, &builtins[i]);
        if (err != OK) {
            return err;
        }
    }
    return OK;
}

/**
 * @brief Searches for a variable across active lexical scopes from most inner to outer one.
 *
 * @details going from the top of the scope stack naturally supports variable
 *          shadowing in nested blocks
 *
 * @param stack      Pointer to the top of the local scope stack.
 * @param identifier Name of the variable to look up.
 * @return Pointer to the matching symtableItem, or NULL if not found in any scope.
 */
symtableItem *findVarInScopes(stackPtr stack, const char *identifier) {
    while (stack != NULL) {
        symtableItem *item = symtableFind(stack->symtable, identifier);
        if (item != NULL) {
            return item;
        }
        stack = stack->next;
    }
    return NULL;
}

/**
 * @brief Checks whether an expression can be assigned or passed to a target data type.
 *
 * @details Handles Any datatype used by builtin functions and marks
 *          the AST node with convertToDouble = true if an implicit Int -> Double
 *          widening conversion is required.
 *
 * @param targetType Expected destination data type.
 * @param exprNode   Evaluated expression AST node.
 * @return true if types are compatible (or implicitly convertible), false otherwise.
 */
static bool canAssignOrConvert(dataType targetType, ASTNode *exprNode) {
    if (exprNode == NULL) {
        return false;
    }
    dataType srcType = exprNode->semanticType;

    /// Without the BOOLTHEN extension, Boolean expressions cannot be stored, passed, or returned
    if (srcType == TYPE_BOOL || srcType == TYPE_UNKNOWN) {
        return false;
    }

    /// Wildcard type Any in print(Any) and toStr(Any) accepts Int, Double, String, or Unit
    if (targetType == TYPE_ANY) {
        return true;
    }

    if (targetType == srcType) {
        return true;
    }

    /// The only allowed implicit conversion in IFJ26: widening Int to Double
    if (targetType == TYPE_DOUBLE && srcType == TYPE_INT) {
        exprNode->convertToDouble = true;
        return true;
    }

    return false;
}

/**
 * @brief Counts the number of formal parameters in a function definition's parameter list.
 *
 * @param param Pointer to the first NODE_PARAM in the linked list.
 * @return Total number of parameters.
 */
static size_t getNumberOfParams(const ASTNode *param) {
    const ASTNode *curr = param;
    size_t sum = 0;
    while (curr != NULL) {
        sum++;
        curr = curr->next;
    }
    return sum;
}

/**
 * @brief Allocates and populates the parameter array for a function symbol table entry.
 *
 * @param funcInTable Target function symbol table item.
 * @param firstParam  First NODE_PARAM in the AST parameter list.
 * @param paramCnt    Number of parameters to copy.
 * @return OK on success, ERR_SEM_REDEFINITION on duplicate parameter names, or ERR_INTERNAL.
 */
static uint8_t fillFuncParams(symtableItem *funcInTable, ASTNode *firstParam, size_t paramCnt) {
    if (paramCnt == 0) {
        funcInTable->data.func.params = NULL;
        return OK;
    }

    funcInTable->data.func.params = calloc(paramCnt, sizeof(*funcInTable->data.func.params));
    if (funcInTable->data.func.params == NULL) {
        return ERR_INTERNAL;
    }

    ASTNode *currParam = firstParam;
    for (size_t i = 0; i < paramCnt; i++) {
        /// Check for duplicate parameter names within the same function header -> Error 4
        for (size_t j = 0; j < i; j++) {
            if (strcmp(funcInTable->data.func.params[j].name, currParam->data.param.name) == 0) {
                return ERR_SEM_REDEFINITION;
            }
        }
        funcInTable->data.func.params[i].name = currParam->data.param.name;
        funcInTable->data.func.params[i].type = currParam->data.param.type;
        currParam = currParam->next;
    }
    return OK;
}

/**
 * @brief Pass 1: Collects all user-defined function headers and validates `@main` rules.
 *
 * @details Registers all function signatures in `globalFuncTable` before function bodies
 *          are analyzed, allowing functions to be called prior to their lexical definition.
 *
 * @param ctx       Pointer to the semantic analysis context.
 * @param functions Pointer to the first NODE_FUNC_DEF in the program.
 * @return OK on success, or an appropriate semantic error code (3, 4, 5, 6, 99).
 */
static uint8_t collectAllFunc(SemanticContext *ctx, ASTNode *functions) {
    bool mainExist = false;
    ASTNode *curr = functions;

    while (curr != NULL) {
        /// Redefinition of an existing user or built-in function -> Error 4
        if (symtableFind(ctx->globalFuncTable, curr->data.funcDef.name) != NULL) {
            return ERR_SEM_REDEFINITION;
        }

        symtableItem *funcInTable =
            symtableLookupAdd(ctx->globalFuncTable, curr->data.funcDef.name);
        if (funcInTable == NULL) {
            return ERR_INTERNAL;
        }

        /// Link the AST node and the symbol table entry in both directions
        funcInTable->kind = SYM_FUNC;
        funcInTable->data.func.node = curr;
        funcInTable->data.func.isBuiltin = false;
        funcInTable->data.func.isEvaluated = false;
        funcInTable->data.func.beingEvaluated = false;
        funcInTable->data.func.hasReturnStmt = false;
        funcInTable->data.func.isMain = curr->data.funcDef.isMain;

        funcInTable->mangledName =
            createMangledFuncName(curr->data.funcDef.name, curr->data.funcDef.isMain);
        if (funcInTable->mangledName == NULL) {
            return ERR_INTERNAL;
        }
        curr->data.funcDef.mangledName = dupCString(funcInTable->mangledName);
        if (curr->data.funcDef.mangledName == NULL) {
            return ERR_INTERNAL;
        }

        size_t paramCnt = getNumberOfParams(curr->data.funcDef.params);
        funcInTable->data.func.paramCount = paramCnt;

        funcInTable->data.func.returnType = curr->data.funcDef.returnType;
        funcInTable->data.func.hasExplicitReturnType =
            (curr->data.funcDef.returnType != TYPE_UNKNOWN);

        /// Enforce constraints for the @main entry function
        if (curr->data.funcDef.isMain) {
            if (mainExist) {
                return ERR_SEM_REDEFINITION; /// Error 4: Multiple @main entry functions
            }
            if (paramCnt != 0) {
                return ERR_SEM_ARGUMENTS; /// Error 5: @main must not have parameters
            }
            if (curr->data.funcDef.returnType != TYPE_UNKNOWN &&
                curr->data.funcDef.returnType != TYPE_UNIT) {
                return ERR_SEM_TYPE; /// Error 6: @main must return Unit
            }
            mainExist = true;
        }

        uint8_t err = fillFuncParams(funcInTable, curr->data.funcDef.params, paramCnt);
        if (err != OK) {
            return err;
        }
        curr = curr->next;
    }

    /// Missing @main entry function in the program -> Error 3
    if (!mainExist) {
        return ERR_SEM_UNDEFINED;
    }
    return OK;
}

static uint8_t evaluateExpresion(
    SemanticContext *ctx, ASTNode *expr, dataType *outType, bool isCond);

/**
 * @brief Validates operand types for the binary '+' operator (numeric addition or string
 * concatenation).
 *
 * @param expr  Binary operator AST node.
 * @param right Right operand AST node.
 * @param left  Left operand AST node.
 * @param rType Evaluated type of the right operand.
 * @param lType Evaluated type of the left operand.
 * @return OK if valid, ERR_SEM_TYPE (6) on incompatible operand types.
 */
static uint8_t evaluateAddOp(
    ASTNode *expr, ASTNode *right, ASTNode *left, dataType rType, dataType lType) {
    if (lType == TYPE_STRING && rType == TYPE_STRING) {
        expr->semanticType = TYPE_STRING;
    } else if (lType == TYPE_INT && rType == TYPE_INT) {
        expr->semanticType = TYPE_INT;
    } else if (lType == TYPE_DOUBLE && rType == TYPE_DOUBLE) {
        expr->semanticType = TYPE_DOUBLE;
    } else if (lType == TYPE_DOUBLE && rType == TYPE_INT) {
        right->convertToDouble = true;
        expr->semanticType = TYPE_DOUBLE;
    } else if (lType == TYPE_INT && rType == TYPE_DOUBLE) {
        left->convertToDouble = true;
        expr->semanticType = TYPE_DOUBLE;
    } else {
        return ERR_SEM_TYPE;
    }
    return OK;
}

/**
 * @brief Validates operand types for arithmetic '-', '*', and '/' operators.
 *
 * @param expr  Binary operator AST node.
 * @param right Right operand AST node.
 * @param left  Left operand AST node.
 * @param rType Evaluated type of the right operand.
 * @param lType Evaluated type of the left operand.
 * @return OK if valid, ERR_SEM_TYPE (6) on non-numeric operand types.
 */
static uint8_t evaluateSubOp(
    ASTNode *expr, ASTNode *right, ASTNode *left, dataType rType, dataType lType) {
    if (lType == TYPE_INT && rType == TYPE_INT) {
        expr->semanticType = TYPE_INT;
    } else if (lType == TYPE_DOUBLE && rType == TYPE_DOUBLE) {
        expr->semanticType = TYPE_DOUBLE;
    } else if (lType == TYPE_DOUBLE && rType == TYPE_INT) {
        right->convertToDouble = true;
        expr->semanticType = TYPE_DOUBLE;
    } else if (lType == TYPE_INT && rType == TYPE_DOUBLE) {
        left->convertToDouble = true;
        expr->semanticType = TYPE_DOUBLE;
    } else {
        return ERR_SEM_TYPE;
    }
    return OK;
}

/**
 * @brief Validates operand types for relational operators '<', '<=', '>', and '>='.
 *
 * @param expr   Binary operator AST node.
 * @param right  Right operand AST node.
 * @param left   Left operand AST node.
 * @param rType  Evaluated type of the right operand.
 * @param lType  Evaluated type of the left operand.
 * @param isCond True if the expression is the root of an `if` or `while` condition.
 * @return OK if valid, ERR_SEM_TYPE (6) otherwise.
 */
static uint8_t evaluateNonEqRelOp(
    ASTNode *expr, ASTNode *right, ASTNode *left, dataType rType, dataType lType, bool isCond) {
    /// Relational operators are only permitted inside conditions without BOOLTHEN
    if (!isCond) {
        return ERR_SEM_TYPE;
    }
    if (lType == TYPE_STRING && rType == TYPE_STRING) {
        expr->semanticType = TYPE_BOOL;
    } else if ((lType == TYPE_DOUBLE || lType == TYPE_INT) &&
               (rType == TYPE_DOUBLE || rType == TYPE_INT)) {
        if (lType == TYPE_INT && rType == TYPE_DOUBLE) {
            left->convertToDouble = true;
        } else if (rType == TYPE_INT && lType == TYPE_DOUBLE) {
            right->convertToDouble = true;
        }
        expr->semanticType = TYPE_BOOL;
    } else {
        return ERR_SEM_TYPE;
    }
    return OK;
}

/**
 * @brief Validates operand types for equality operators '==' and '!='.
 *
 * @param expr   Binary operator AST node.
 * @param right  Right operand AST node.
 * @param left   Left operand AST node.
 * @param rType  Evaluated type of the right operand.
 * @param lType  Evaluated type of the left operand.
 * @param isCond True if the expression is the root of an `if` or `while` condition.
 * @return OK if valid, ERR_SEM_TYPE (6) otherwise.
 */
static uint8_t evaluateEq(
    ASTNode *expr, ASTNode *right, ASTNode *left, dataType rType, dataType lType, bool isCond) {
    if (!isCond) {
        return ERR_SEM_TYPE;
    }
    if (lType == rType && lType != TYPE_BOOL && lType != TYPE_UNKNOWN) {
        expr->semanticType = TYPE_BOOL;
        return OK;
    }
    /// Allow comparing Int and Double via implicit Int -> Double promotion
    if (lType == TYPE_INT && rType == TYPE_DOUBLE) {
        left->convertToDouble = true;
        expr->semanticType = TYPE_BOOL;
        return OK;
    }
    if (lType == TYPE_DOUBLE && rType == TYPE_INT) {
        right->convertToDouble = true;
        expr->semanticType = TYPE_BOOL;
        return OK;
    }
    return ERR_SEM_TYPE;
}

/**
 * @brief Evaluates a binary operation node (`NODE_BIN_OP`) and infers its resulting type.
 *
 * @param ctx     Pointer to the semantic analysis context.
 * @param expr    Binary operation AST node.
 * @param outType Output pointer receiving the resulting type of the expression.
 * @param isCond  True if evaluating the top-level condition of an `if` or `while` statement.
 * @return OK on success, or an error code on invalid sub-expressions or operand types.
 */
static uint8_t evaluateBinOp(SemanticContext *ctx, ASTNode *expr, dataType *outType, bool isCond) {
    ASTNode *left = expr->data.binOp.left;
    ASTNode *right = expr->data.binOp.right;
    dataType lType = TYPE_UNKNOWN;
    dataType rType = TYPE_UNKNOWN;

    /// Sub-operands of a binary operator cannot be relational expressions themselves (without
    /// BOOLTHEN)
    uint8_t err = evaluateExpresion(ctx, left, &lType, false);
    if (err != OK) {
        return err;
    }
    err = evaluateExpresion(ctx, right, &rType, false);
    if (err != OK) {
        return err;
    }

    BinaryOpType opType = expr->data.binOp.op;

    if (opType == OP_ADD) {
        err = evaluateAddOp(expr, right, left, rType, lType);
    } else if (opType == OP_SUB || opType == OP_MUL || opType == OP_DIV) {
        err = evaluateSubOp(expr, right, left, rType, lType);
    } else if (opType == OP_LT || opType == OP_LTE || opType == OP_GT || opType == OP_GTE) {
        err = evaluateNonEqRelOp(expr, right, left, rType, lType, isCond);
    } else if (opType == OP_EQ || opType == OP_NEQ) {
        err = evaluateEq(expr, right, left, rType, lType, isCond);
    } else {
        return ERR_INTERNAL;
    }

    if (err != OK) {
        return err;
    }

    *outType = expr->semanticType;
    return OK;
}

static uint8_t checkFunction(symtableItem *func, SemanticContext *ctx);

/**
 * @brief Validates a function call (`NODE_FUNC_CALL`), checking argument count and types.
 *
 * @details If the called function has no explicit return type in its header and has not
 *          been analyzed yet, triggers on-demand semantic analysis of the target function.
 *
 * @param ctx     Pointer to the semantic analysis context.
 * @param expr    Function call AST node.
 * @param outType Output pointer receiving the return type of the called function.
 * @return OK on success, ERR_SEM_UNDEFINED (3), ERR_SEM_ARGUMENTS (5), or ERR_SEM_TYPE (6).
 */
static uint8_t evaluateFuncCall(SemanticContext *ctx, ASTNode *expr, dataType *outType) {
    symtableItem *func = symtableFind(ctx->globalFuncTable, expr->data.funcCall.funcName);
    if (func == NULL) {
        return ERR_SEM_UNDEFINED; /// Error 3: Calling an undefined function
    }

    /// 1. Evaluate all argument expressions and count them
    size_t argCount = 0;
    ASTNode *arg = expr->data.funcCall.args;
    while (arg != NULL) {
        dataType argType = TYPE_UNKNOWN;
        uint8_t err = evaluateExpresion(ctx, arg, &argType, false);
        if (err != OK) {
            return err;
        }
        argCount++;
        arg = arg->next;
    }

    /// 2. Verify argument count -> Error 5 on mismatch
    if (argCount != func->data.func.paramCount) {
        return ERR_SEM_ARGUMENTS;
    }

    /// 3. Verify argument types against formal parameters -> Error 5 on mismatch
    arg = expr->data.funcCall.args;
    for (size_t i = 0; i < argCount; i++) {
        if (!canAssignOrConvert(func->data.func.params[i].type, arg)) {
            return ERR_SEM_ARGUMENTS;
        }
        arg = arg->next;
    }

    /// 4. If the callee's return type must be inferred, analyze the callee now (on-demand)
    if (!func->data.func.hasExplicitReturnType && !func->data.func.isEvaluated) {
        uint8_t err = checkFunction(func, ctx);
        if (err != OK) {
            return err;
        }
    }

    if (expr->data.funcCall.mangledName == NULL) {
        if (func->data.func.isBuiltin) {
            expr->data.funcCall.mangledName = dupCString(expr->data.funcCall.funcName);
        } else {
            expr->data.funcCall.mangledName = dupCString(func->mangledName);
        }
        if (expr->data.funcCall.mangledName == NULL) {
            return ERR_INTERNAL;
        }
    }

    expr->semanticType = func->data.func.returnType;
    *outType = func->data.func.returnType;
    return OK;
}

/**
 * @brief Evaluates a literal node (`NODE_LITERAL`) by copying its literal type to `semanticType`.
 *
 * @param expr    Literal AST node.
 * @param outType Output pointer receiving the literal's data type.
 * @return Always OK.
 */
static uint8_t evaluateLiteral(ASTNode *expr, dataType *outType) {
    expr->semanticType = expr->data.literal.litType;
    *outType = expr->semanticType;
    return OK;
}

/**
 * @brief Evaluates a variable reference (`NODE_VAR_REF`), verifying it is defined in scope.
 *
 * @param ctx     Pointer to the semantic analysis context.
 * @param expr    Variable reference AST node.
 * @param outType Output pointer receiving the variable's data type.
 * @return OK if found, ERR_SEM_UNDEFINED (3) if the variable is undefined.
 */
static uint8_t evaluateVarRef(SemanticContext *ctx, ASTNode *expr, dataType *outType) {
    symtableItem *item = findVarInScopes(ctx->localScopeStack, expr->data.varRef.name);
    if (item == NULL) {
        return ERR_SEM_UNDEFINED; /// Error 3: Undefined variable
    }
    if (expr->data.varRef.mangledName == NULL) {
        expr->data.varRef.mangledName = dupCString(item->mangledName);
        if (expr->data.varRef.mangledName == NULL) {
            return ERR_INTERNAL;
        }
    }
    expr->semanticType = item->data.var.type;
    *outType = item->data.var.type;
    return OK;
}

/**
 * @brief Evaluates a unary operator (`NODE_UNARY_OP`, `+x` or `-x`).
 *
 * @param ctx     Pointer to the semantic analysis context.
 * @param expr    Unary operator AST node.
 * @param outType Output pointer receiving the operand's numeric type.
 * @return OK if the operand is Int or Double, ERR_SEM_TYPE (6) otherwise.
 */
static uint8_t evaluateUnOp(SemanticContext *ctx, ASTNode *expr, dataType *outType) {
    dataType opType = TYPE_UNKNOWN;
    /// Evaluate the inner operand, NOT `expr` itself!
    uint8_t err = evaluateExpresion(ctx, expr->data.unaryOp.operand, &opType, false);
    if (err != OK) {
        return err;
    }
    if (opType != TYPE_INT && opType != TYPE_DOUBLE) {
        return ERR_SEM_TYPE; /// Error 6: Unary +/- requires numeric operand
    }
    expr->semanticType = opType;
    *outType = opType;
    return OK;
}

/**
 * @brief Main dispatcher for evaluating an expression subtree and inferring its type.
 *
 * @param ctx     Pointer to the semantic analysis context.
 * @param expr    Root node of the expression to evaluate.
 * @param outType Output pointer receiving the resulting type of the expression.
 * @param isCond  True if the expression is the top-level condition of an `if` or `while`.
 * @return OK on success, or the corresponding semantic error code.
 */
static uint8_t evaluateExpresion(
    SemanticContext *ctx, ASTNode *expr, dataType *outType, bool isCond) {
    if (expr == NULL) {
        return ERR_INTERNAL;
    }

    switch (expr->type) {
    case NODE_LITERAL:
        return evaluateLiteral(expr, outType);

    case NODE_VAR_REF:
        return evaluateVarRef(ctx, expr, outType);

    case NODE_UNARY_OP:
        return evaluateUnOp(ctx, expr, outType);

    case NODE_BIN_OP:
        return evaluateBinOp(ctx, expr, outType, isCond);

    case NODE_FUNC_CALL:
        return evaluateFuncCall(ctx, expr, outType);

    default:
        return ERR_SEM_OTHER;
    }
}

/**
 * @brief Checks a variable declaration (`val` or `var`), infers or verifies its type,
 *        and inserts it into the current innermost lexical scope.
 *
 * @param ctx  Pointer to the semantic analysis context.
 * @param decl Variable declaration AST node (`NODE_VAR_DECL`).
 * @return OK on success, ERR_SEM_REDEFINITION (4), ERR_SEM_TYPE (6), or ERR_INTERNAL (99).
 */
static uint8_t checkVarDecl(SemanticContext *ctx, ASTNode *decl) {
    Symtable *currScope = ctx->localScopeStack->symtable;

    /// Redefinition in the exact same block (or shadowing a function parameter in top body) ->
    /// Error
    /// 4
    if (symtableFind(currScope, decl->data.varDecl.name) != NULL) {
        return ERR_SEM_REDEFINITION;
    }

    /// Evaluate the right-hand side initialization expression BEFORE adding the variable to the
    /// scope
    dataType rightHandType = TYPE_UNKNOWN;
    uint8_t err = evaluateExpresion(ctx, decl->data.varDecl.initExpr, &rightHandType, false);
    if (err != OK) {
        return err;
    }

    dataType finalType = decl->data.varDecl.varType;

    if (finalType == TYPE_UNKNOWN) {
        /// Type inference: cannot infer from unknown or store a Boolean without BOOLTHEN
        if (rightHandType == TYPE_BOOL || rightHandType == TYPE_UNKNOWN) {
            return ERR_SEM_TYPE;
        }
        finalType = rightHandType;
        decl->data.varDecl.varType = rightHandType;
    } else {
        /// Explicit type annotation: check compatibility (allows Int -> Double conversion)
        if (!canAssignOrConvert(finalType, decl->data.varDecl.initExpr)) {
            return ERR_SEM_TYPE;
        }
    }

    symtableItem *var = symtableLookupAdd(currScope, decl->data.varDecl.name);
    if (var == NULL) {
        return ERR_INTERNAL;
    }

    char *mangled =
        createMangledVarName(ctx->currentFunc->key, decl->data.varDecl.name, ctx->nextVarId);
    ctx->nextVarId++;
    if (mangled == NULL) {
        return ERR_INTERNAL;
    }

    var->kind = SYM_VAR;
    var->data.var.type = finalType;
    var->data.var.isMutable = decl->data.varDecl.isMutable;
    var->mangledName = mangled;

    decl->data.varDecl.mangledName = dupCString(mangled);
    if (decl->data.varDecl.mangledName == NULL) {
        return ERR_INTERNAL;
    }

    uint8_t listErr = appendFuncVar(
        ctx, decl->data.varDecl.name, mangled, finalType, decl->data.varDecl.isMutable);
    if (listErr != OK) {
        return listErr;
    }
    decl->semanticType = TYPE_UNIT;
    return OK;
}

/**
 * @brief Validates an assignment statement (`NODE_ASSIGN`), checking mutability and type
 * compatibility.
 *
 * @param ctx    Pointer to the semantic analysis context.
 * @param assign Assignment AST node.
 * @return OK on success, ERR_SEM_UNDEFINED (3), ERR_SEM_TYPE (6), or ERR_SEM_IMMUTABLE (7).
 */
static uint8_t checkAssign(SemanticContext *ctx, ASTNode *assign) {
    symtableItem *var = findVarInScopes(ctx->localScopeStack, assign->data.assign.targetName);
    if (var == NULL) {
        return ERR_SEM_UNDEFINED; /// Error 3: Assigning to an undefined variable
    }
    if (!var->data.var.isMutable) {
        return ERR_SEM_IMMUTABLE; /// Error 7: Assigning to a 'val' or formal parameter
    }

    dataType exprType = TYPE_UNKNOWN;
    uint8_t err = evaluateExpresion(ctx, assign->data.assign.expr, &exprType, false);
    if (err != OK) {
        return err;
    }

    if (!canAssignOrConvert(var->data.var.type, assign->data.assign.expr)) {
        return ERR_SEM_TYPE; /// Error 6: Incompatible type in assignment
    }
    if (assign->data.assign.mangledName == NULL) {
        assign->data.assign.mangledName = dupCString(var->mangledName);
        if (assign->data.assign.mangledName == NULL) {
            return ERR_INTERNAL;
        }
    }
    assign->semanticType = TYPE_UNIT;
    return OK;
}

/**
 * @brief Validates a `return` statement (`NODE_RETURN`) against the enclosing function's header.
 *
 * @details Per Section 2.3 of the IFJ26 specification, any use of `return` requires the
 *          enclosing function to have an explicit return type annotation in its header.
 *
 * @param ctx Pointer to the semantic analysis context.
 * @param ret Return statement AST node.
 * @return OK on success, or ERR_SEM_TYPE (6) if return rules are violated.
 */
static uint8_t checkReturn(SemanticContext *ctx, ASTNode *ret) {
    symtableItem *func = ctx->currentFunc;

    /// Rule (Sec. 2.3): A function containing 'return' MUST have an explicit return type in its
    /// header
    if (!func->data.func.hasExplicitReturnType) {
        return ERR_SEM_TYPE;
    }

    ASTNode *expr = ret->data.returnStmt.expr;
    dataType retType = func->data.func.returnType;

    /// Bare 'return' without an expression is only valid in a function explicitly returning Unit
    if (expr == NULL) {
        if (retType != TYPE_UNIT) {
            return ERR_SEM_TYPE;
        }
        func->data.func.hasReturnStmt = true;
        ret->semanticType = TYPE_UNIT;
        return OK;
    }

    dataType exprType = TYPE_UNKNOWN;
    uint8_t err = evaluateExpresion(ctx, expr, &exprType, false);
    if (err != OK) {
        return err;
    }

    /// Rule 4 (Sec. 2.3): In a function explicitly returning Unit, 'return expr' evaluates
    /// the expression and discards its value (unless it is an invalid Boolean expression)
    if (retType == TYPE_UNIT) {
        if (exprType == TYPE_BOOL) {
            return ERR_SEM_TYPE;
        }
    } else if (!canAssignOrConvert(retType, expr)) {
        return ERR_SEM_TYPE;
    }

    func->data.func.hasReturnStmt = true;
    ret->semanticType = TYPE_UNIT;
    return OK;
}

static uint8_t checkBlock(SemanticContext *ctx, ASTNode *block);

/**
 * @brief Validates an `if-then-else` statement (`NODE_IF`) and both of its branch blocks.
 *
 * @param ctx  Pointer to the semantic analysis context.
 * @param stmt If statement AST node.
 * @return OK on success, or a semantic error code.
 */
static uint8_t checkIfStmt(SemanticContext *ctx, ASTNode *stmt) {
    dataType condType = TYPE_UNKNOWN;
    uint8_t err = evaluateExpresion(ctx, stmt->data.ifStmt.condition, &condType, true);
    if (err != OK) {
        return err;
    }

    if (condType != TYPE_BOOL) {
        return ERR_SEM_TYPE; /// Error 6: Condition is not a relational expression
    }

    err = checkBlock(ctx, stmt->data.ifStmt.thenBlock);
    if (err != OK) {
        return err;
    }

    err = checkBlock(ctx, stmt->data.ifStmt.elseBlock);
    if (err != OK) {
        return err;
    }

    stmt->data.ifStmt.idx = ctx->nextIfLabelId++;
    stmt->semanticType = TYPE_UNIT;
    return OK;
}

/**
 * @brief Validates a `while` loop (`NODE_WHILE`), its condition, and its body block.
 *
 * @param ctx  Pointer to the semantic analysis context.
 * @param stmt While statement AST node.
 * @return OK on success, or a semantic error code.
 */
static uint8_t checkWhileStmt(SemanticContext *ctx, ASTNode *stmt) {
    dataType condType = TYPE_UNKNOWN;
    uint8_t err = evaluateExpresion(ctx, stmt->data.whileStmt.condition, &condType, true);
    if (err != OK) {
        return err;
    }

    if (condType != TYPE_BOOL) {
        return ERR_SEM_TYPE; /// Error 6: Condition is not a relational expression
    }

    err = checkBlock(ctx, stmt->data.whileStmt.bodyBlock);
    if (err != OK) {
        return err;
    }

    stmt->data.whileStmt.idx = ctx->nextwhileLabelId++;
    stmt->semanticType = TYPE_UNIT;
    return OK;
}

/**
 * @brief Dispatches semantic checking for a single statement inside a block.
 *
 * @param ctx  Pointer to the semantic analysis context.
 * @param stmt Statement AST node to validate.
 * @return OK on success, or a semantic error code.
 */
static uint8_t checkStmt(SemanticContext *ctx, ASTNode *stmt) {
    if (stmt == NULL) {
        return OK;
    }

    switch (stmt->type) {
    case NODE_IF:
        return checkIfStmt(ctx, stmt);
    case NODE_WHILE:
        return checkWhileStmt(ctx, stmt);
    case NODE_RETURN:
        return checkReturn(ctx, stmt);
    case NODE_VAR_DECL:
        return checkVarDecl(ctx, stmt);
    case NODE_ASSIGN:
        return checkAssign(ctx, stmt);
    case NODE_BLOCK:
        return checkBlock(ctx, stmt);
    default: {
        /// Standalone expression or function call acting as a statement in a block
        dataType exprType = TYPE_UNKNOWN;
        return evaluateExpresion(ctx, stmt, &exprType, false);
    }
    }
}

/**
 * @brief Opens a new lexical scope, validates all statements in a block, and pops the scope.
 *
 * @details Uses `unwrapBlock` so that a new scope is created and all `->next` statements
 *          are iterated whether `block` is wrapped in `NODE_BLOCK` or points directly
 *          to the first statement of the branch/body.
 *
 * @param ctx   Pointer to the semantic analysis context.
 * @param block Pointer to the block wrapper or the first statement of the block.
 * @return OK on success, or the first encountered semantic error code.
 */
static uint8_t checkBlock(SemanticContext *ctx, ASTNode *block) {
    ASTNode *curr = unwrapBlock(block);
    if (curr == NULL) {
        return OK;
    }

    /// Create and push a new symbol table for this block's lexical scope (prime capacity 37)
    Symtable *scope = symtableInit(37);
    if (scope == NULL) {
        return ERR_INTERNAL;
    }
    push(scope, &ctx->localScopeStack);

    uint8_t err = OK;
    while (curr != NULL) {
        err = checkStmt(ctx, curr);
        if (err != OK) {
            break;
        }
        curr = curr->next;
    }

    pop(&ctx->localScopeStack);
    /// Note: If your pop() implementation does not free `scope` internally, call
    /// `symtableFree(scope);` here!
    return err;
}

/**
 * @brief Inspects the last statement of a block to determine the block's result type
 *        and whether all execution paths end with an explicit `return`.
 *
 * @details Implements Rules 1-3 from Section 2.3 (page 6) of the IFJ26 specification.
 *
 * @param body         Pointer to the function/branch body (`NODE_BLOCK` or first statement).
 * @param func         Symbol table entry of the enclosing function.
 * @param outAllreturn Output flag set to true if all paths end with `NODE_RETURN`.
 * @param outType      Output pointer receiving the resulting type of the block's tail.
 * @param outLastExpr  Output pointer receiving the final expression node (for Int -> Double
 * conversion).
 * @return OK on success, or ERR_SEM_TYPE (6) if an `if-else` tail violates Rule 3.
 */
static uint8_t checkFunctionTail(ASTNode *body, symtableItem *func, bool *outAllreturn,
    dataType *outType, ASTNode **outLastExpr) {
    *outLastExpr = NULL;
    ASTNode *curr = unwrapBlock(body);

    if (curr == NULL) {
        *outAllreturn = false;
        *outType = TYPE_UNIT;
        return OK;
    }

    /// Advance to the very last statement in the block's linked list
    while (curr->next != NULL) {
        curr = curr->next;
    }

    switch (curr->type) {
    case NODE_RETURN:
        *outAllreturn = true;
        *outType = TYPE_UNIT;
        return OK;

    /// Rule 2: A block ending with a variable declaration, assignment, or while loop has result ()
    /// (Unit)
    case NODE_WHILE:
    case NODE_ASSIGN:
    case NODE_VAR_DECL:
        *outAllreturn = false;
        *outType = TYPE_UNIT;
        return OK;

    /// Rule 3: A block ending with an if-else statement
    case NODE_IF: {
        dataType thenType = TYPE_UNIT;
        dataType elseType = TYPE_UNIT;
        bool thenRet = false;
        bool elseRet = false;
        ASTNode *noUse = NULL;

        uint8_t err =
            checkFunctionTail(curr->data.ifStmt.thenBlock, func, &thenRet, &thenType, &noUse);
        if (err != OK) {
            return err;
        }

        err = checkFunctionTail(curr->data.ifStmt.elseBlock, func, &elseRet, &elseType, &noUse);
        if (err != OK) {
            return err;
        }

        *outAllreturn = (bool)(thenRet && elseRet);

        if (*outAllreturn) {
            *outType = func->data.func.returnType;
            return OK;
        }

        if (func->data.func.hasExplicitReturnType && func->data.func.returnType == TYPE_UNIT) {
            *outType = TYPE_UNIT;
            return OK;
        }

        if (!func->data.func.hasExplicitReturnType && thenType == TYPE_UNIT &&
            elseType == TYPE_UNIT) {
            *outType = TYPE_UNIT;
            return OK;
        }
        return ERR_SEM_TYPE; /// Error 6: Invalid if-else at the end of a function
    }

    /// Rule 2: A block ending with an expression or function call returns that expression's value
    default:
        *outAllreturn = false;
        *outType = curr->semanticType;
        *outLastExpr = curr;
        return OK;
    }
}

/**
 * @brief Verifies or infers a function's return type after its body has been checked.
 *
 * @param func     Symbol table entry of the analyzed function.
 * @param funcNode AST node of the function definition (`NODE_FUNC_DEF`).
 * @return OK on success, or ERR_SEM_TYPE (6) on return type mismatch / failed inference.
 */
static uint8_t checkFuncReturn(symtableItem *func, ASTNode *funcNode) {
    dataType outType = TYPE_UNIT;
    ASTNode *lastExpr = NULL;
    bool allPathsReturn = false;

    uint8_t err =
        checkFunctionTail(funcNode->data.funcDef.body, func, &allPathsReturn, &outType, &lastExpr);
    if (err != OK) {
        return err;
    }

    /// If all execution paths ended with an explicit 'return', checkReturn() already validated them
    if (allPathsReturn) {
        return OK;
    }

    if (func->data.func.hasExplicitReturnType) {
        /// Rule 4: In a function with explicit return type Unit, the trailing expression value is
        /// discarded
        if (func->data.func.returnType == TYPE_UNIT) {
            if (outType == TYPE_BOOL) {
                return ERR_SEM_TYPE;
            }
            return OK;
        }
        /// Function explicitly returns Int/Double/String -> must end with a compatible expression
        if (lastExpr == NULL || !canAssignOrConvert(func->data.func.returnType, lastExpr)) {
            return ERR_SEM_TYPE;
        }
    } else {
        /// Return type was omitted -> infer it from the last statement in the body
        if (outType == TYPE_BOOL || outType == TYPE_UNKNOWN) {
            return ERR_SEM_TYPE;
        }
        /// If @main had no explicit return type, its inferred return type must still be Unit
        if (func->data.func.isMain && outType != TYPE_UNIT) {
            return ERR_SEM_TYPE;
        }
        func->data.func.returnType = outType;
        funcNode->data.funcDef.returnType = outType;
    }
    return OK;
}

/**
 * @brief Analyzes a single function definition, its parameter scope, body statements, and return
 * type.
 *
 * @details Supports on-demand evaluation when a function without an explicit return type is
 *          called before its definition, and detects illegal recursion without an explicit return
 * type.
 *
 * @param func Symbol table entry of the function to analyze.
 * @param ctx  Pointer to the semantic analysis context.
 * @return OK on success, or a semantic error code.
 */
static uint8_t checkFunction(symtableItem *func, SemanticContext *ctx) {
    if (func->data.func.isEvaluated) {
        return OK;
    }

    /// Detect direct or mutual recursion: recursive functions MUST have an explicit return type
    if (func->data.func.beingEvaluated) {
        if (!func->data.func.hasExplicitReturnType) {
            return ERR_SEM_TYPE; /// Error 6
        }
        return OK;
    }

    func->data.func.beingEvaluated = true;
    ASTNode *funcNode = func->data.func.node;

    /// Back up current context in case checkFunction was triggered mid-expression from another
    /// function
    symtableItem *prevFunc = ctx->currentFunc;
    stackPtr prevStack = ctx->localScopeStack;
    FuncVarEntry *prevFuncTail = ctx->varListTail;
    size_t prevVarId = ctx->nextVarId;

    ctx->currentFunc = func;
    ctx->localScopeStack = NULL;
    ctx->nextVarId = 0;
    ctx->varListTail = NULL;

    funcNode->data.funcDef.vars = NULL;
    funcNode->data.funcDef.varCount = 0;

    /// Create the top-level scope of the function body and insert formal parameters as immutable
    /// variables
    Symtable *scope = symtableInit(37);
    if (scope == NULL) {
        func->data.func.beingEvaluated = false;
        ctx->currentFunc = prevFunc;
        ctx->localScopeStack = prevStack;
        ctx->varListTail = prevFuncTail;
        ctx->nextVarId = prevVarId;
        return ERR_INTERNAL;
    }
    push(scope, &ctx->localScopeStack);

    uint8_t err = OK;
    for (size_t i = 0; i < func->data.func.paramCount; i++) {
        symtableItem *paramVar = symtableLookupAdd(scope, func->data.func.params[i].name);
        if (paramVar == NULL) {
            err = ERR_INTERNAL;
            break;
        }
        char *paramMangled =
            createMangledVarName(func->key, func->data.func.params[i].name, ctx->nextVarId);
        ctx->nextVarId++;
        if (paramMangled == NULL) {
            err = ERR_INTERNAL;
            break;
        }
        ASTNode *currParam = funcNode->data.funcDef.params;
        paramVar->kind = SYM_VAR;
        paramVar->data.var.type = func->data.func.params[i].type;
        paramVar->data.var.isMutable = false;
        paramVar->mangledName = paramMangled;

        err = appendFuncVar(ctx, func->data.func.params[i].name, paramMangled,
            func->data.func.params[i].type, false);
        if (err != OK) {
            break;
        }
        if (currParam != NULL) {
            currParam->data.param.mangledName = dupCString(paramMangled);
            if (currParam->data.param.mangledName == NULL) {
                err = ERR_INTERNAL;
                break;
            }
            currParam = currParam->next;
        }
    }

    /// unwrapBlock handles both NODE_BLOCK and a direct pointer to the first statement
    ASTNode *curr = unwrapBlock(funcNode->data.funcDef.body);
    while (err == OK && curr != NULL) {
        err = checkStmt(ctx, curr);
        curr = curr->next;
    }

    pop(&ctx->localScopeStack);
    ctx->currentFunc = prevFunc;
    ctx->localScopeStack = prevStack;
    ctx->varListTail = prevFuncTail;
    ctx->nextVarId = prevVarId;

    if (err != OK) {
        func->data.func.beingEvaluated = false;
        return err;
    }

    err = checkFuncReturn(func, funcNode);
    func->data.func.beingEvaluated = false;

    if (err == OK) {
        func->data.func.isEvaluated = true;
    }
    return err;
}

/**
 * @brief Main entry point for the semantic analysis phase.
 *
 * @details Executes a two-pass verification over the AST:
 *          1. Registers built-in functions and collects all user function headers.
 *          2. Performs deep semantic and type checking on every function body.
 *
 * @param program Pointer to the root `NODE_PROGRAM` AST node.
 * @return OK (0) if the program is semantically valid, or an IFJ26 error code (3-7, 10, 99).
 */
uint8_t semantAnalyse(ASTNode *program) {
    if (program == NULL || program->type != NODE_PROGRAM) {
        return ERR_INTERNAL;
    }

    SemanticContext ctx = {
        .globalFuncTable = symtableInit(67),
        .currentFunc = NULL,
        .localScopeStack = NULL,
        .nextIfLabelId = 0,
        .nextwhileLabelId = 0,
        .nextVarId = 0,
    };
    if (ctx.globalFuncTable == NULL) {
        return ERR_INTERNAL;
    }

    /// Pass 1: Register built-ins and collect all function signatures
    uint8_t err = registerBuiltins(ctx.globalFuncTable);
    if (err == OK) {
        err = collectAllFunc(&ctx, program->data.program.functions);
    }

    /// Pass 2: Validate all user-defined function bodies
    ASTNode *curr = program->data.program.functions;
    while (err == OK && curr != NULL) {
        symtableItem *func = symtableFind(ctx.globalFuncTable, curr->data.funcDef.name);
        if (func == NULL) {
            return ERR_INTERNAL;
        }
        err = checkFunction(func, &ctx);
        curr = curr->next;
    }
    symtableFree(ctx.globalFuncTable);
    return err;
}
