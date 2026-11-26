/**
 * @file    symtable.h
 * @brief   Header file for symbol table data type (Hashtable - open addressing)
 * @author  Jakub Poláček (xpolacj00)
 * @details project_IFJ2026
 */

#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h> // size_t

typedef struct ASTNode ASTNode;
typedef const char *symtableKeyT;

typedef enum : uint8_t {
    TYPE_UNKNOWN, // not yet known return type
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_UNIT,
    TYPE_ANY,
    TYPE_BOOL, // internal type for conditions
} dataType;

typedef enum : uint8_t {
    SYM_VAR,  // Variable
    SYM_FUNC, // User defined or buildin functions
} symKind;

// State of one field in our array
typedef enum : uint8_t {
    SLOT_EMPTY,    // Never used field
    SLOT_OCCUPIED, // Currently Occupied (active)
    SLOT_DELETED,  // We deleted the symbol but we need to raise this flag for searching
} slotState;

// structure for parametrs
typedef struct {
    char *name;
    dataType type;
} funcParam;

// Data for variables
typedef struct {
    dataType type;
    bool isMutable; // false for constants
    bool used;
} symVarData;

typedef struct {
    dataType returnType;
    ASTNode *node;
    bool hasExplicitReturnType; // was the return type explicitly stated
    bool hasReturnStmt;         // does the body contain return type?
    bool beingEvaluated;
    bool isEvaluated;
    bool isMain;
    bool isBuiltin;
    size_t paramCount;
    funcParam *params; // Dynamic field of params
} symFuncData;

typedef struct {
    char *key;         // identificator name
    char *mangledName; /// mangled name code for code generator
    slotState state;
    symKind kind; // SYM_VAR or SYM_FUNC
    union {
        symVarData var;
        symFuncData func;
    } data;
} symtableItem;

typedef struct SymtableT {
    size_t size;      // SLOT_OCCUPIRED
    size_t usedSlots; // SLOT_OCCUPIED + SLOT_DELETED (to decide if we should rehash)
    size_t capacity;
    symtableItem *items;
} Symtable;

/**
 * @brief Initilizes Symtable with n items
 * @param n number of
 * @return Pointer to the new initalized Symtable
 */
Symtable *symtableInit(size_t n);

/**
 * @brief Finds item in symtable
 * @param symtable target symtable
 * @param key string which you want to add as a key
 * @return Pointer to the new added or found item or NULL in case of not found
 * or init error
 */
symtableItem *symtableFind(Symtable *table, symtableKeyT key);

/**
 * @brief Adds item in symtable or finds it when
 * @param symtable target symtable
 * @param key string which you want to add as a key
 * @return Pointer to the new added or found item or NULL in case of not found
 * or init error
 */
symtableItem *symtableLookupAdd(Symtable *table, symtableKeyT key);

/**
 * @brief Frees symtable with n items
 * @param table pointer to the to be deleted table
 */
void symtableFree(Symtable *table);

/**
 * @brief Debugging function for printing all keys stored in array
 * @param table
 */
void symtablePrint(Symtable const *table);
#endif // SYMTAB_H
