/**
 * @file    symtableStack.c
 * @brief   Source file for semanthical analysis
 * @author  Jakub Poláček (xpolacj00)
 * @date    2026-11-6
 * @details project: IFJ2026
 */
#include "semantAnalys.h"
#include "symtable.h"
#include <stddef.h>
#include <stdlib.h>

#include "symtableStack.h"

// Chybové kódy podle zadání IFJ (příklad)
#define SEM_OK 0
#define ERR_SEM_UNDEF 3 // Nedefinovaná funkce / proměnná, redefinice
#define ERR_SEM_PARAMS                                                         \
    4 // Špatný počet/typ parametrů u volání funkce nebo return
#define ERR_SEM_UNINIT_CONST 5 // Přepis konstanty / neinicializovaná proměnná
#define ERR_SEM_RETURN 6       // Chybějící / přebývající výraz u return
#define ERR_SEM_TYPE_COMPAT 7  // Typová nekompatibilita v aritmetice/relacích
#define ERR_SEM_TYPE_INFER 8   // Nelze odvodit typ proměnné
#define ERR_SEM_OTHER 9

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

bool checkForUnusedVar(stackPtr stack) {
    if (stack == NULL) {
        return true;
    }
    for (size_t i = 0; i < stack->symtable->capacity; i++) {
        if (stack->symtable->items[i].state == SLOT_EMPTY) {
            continue;
        }
        if (!stack->symtable->items[i].data.var.used) {
            return true;
        }
    }
    return false;
}

dataType getDataType(stackPtr stack, const char *identifier) {
    while (stack != NULL) {
        symtableItem *item = symtableFind(stack->symtable, identifier);
        if (item != NULL) {
            return item->data.var.type;
        }
        stack = stack->next;
    }
    return NULL;
}

dataType getFuncReturnType(stackPtr funcStack, const char *identifier) {
    symtableItem *item = symtableFind(funcStack->symtable, identifier);
    if (item != NULL) {
        return item->data.func.returnType;
    }
    return NULL;
}

int evaluateExpresion(SemanticContext *ctx, ASTNode *expr, dataType *outType) {
    if (expr == NULL) {
        return ERR_SEM_OTHER;
    }
    switch (expr->type) {
    case NODE_LITERAL:
        expr->semanticType = expr->data.literal.litType;
        *outType = expr->semanticType;
        return SEM_OK;

    case NODE_VAR_REF:
        symtableItem *item =
            findVarInScopes(ctx->localScopeStack, expr->data.varRef.name);
        if (item == NULL) {
            return ERR_SEM_UNDEF;
        }
        expr->semanticType = item->data.var.type;
        expr->symbol = item;
        *outType = item->data.var.type;
        return SEM_OK;

    case NODE_FUNC_CALL:
        symtableItem *func =
            symtableFind(ctx->globalFuncTable, expr->data.funcCall.funcName);
        if (func == NULL) {
            return ERR_SEM_UNDEF;
        }
        ASTNode *param = expr->data.funcCall.args;
        size_t paramNum = func->data.func.paramCount;
        if (param != NULL && paramNum != 0) {
            size_t idx = paramNum;
            while (param != NULL && idx != 0) {
                if (param->data.param.type !=
                    func->data.func.params[paramNum - idx].type) {
                    return ERR_SEM_PARAMS;
                }
                idx--;
                param = param->next;
            }
        } else {
            if (param == NULL && paramNum == 0) {
            } else {
                return ERR_SEM_PARAMS;
            }
        }
        expr->semanticType = func->data.func.returnType;
        expr->symbol = func;
        *outType = func->data.func.returnType;
        return SEM_OK;

    default:
        return ERR_SEM_OTHER;
    }
}
