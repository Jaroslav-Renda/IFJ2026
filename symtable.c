/**
 * @file    symtable.c
 * @brief   Source file for symbol table data type (Hashtable - open addressing)
 * @author  Jakub Poláček (xpolacj00)
 * @details project_IFJ2026
 */

#include "symtable.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REHASH_LIMIT 6
// representing 60% of used slots in the table, if this limit is reached, rehashing will be
// performed

Symtable *symtableInit(size_t n) {
    if (n < 1) {
        return NULL;
    }
    Symtable *tab = malloc(sizeof(Symtable));
    if (tab == NULL) {
        return NULL;
    }
    tab->items = malloc(sizeof(symtableItem) * n);
    if (tab->items == NULL) {
        free(tab);
        return NULL;
    }
    for (size_t i = 0; i < n; i++) {
        tab->items[i].state = SLOT_EMPTY;
        tab->items[i].key = NULL;
    }
    tab->size = 0;
    tab->usedSlots = 0;
    tab->capacity = n;
    return tab;
}

void symtableFree(Symtable *table) {
    if (table == NULL) {
        return;
    }
    for (size_t i = 0; i < table->capacity; i++) {
        if (table->items[i].key != NULL && table->items[i].kind == SYM_FUNC) {
            free(table->items[i].data.func.params);
        }
        free(table->items[i].key);
    }
    free(table->items);
    free(table);
}

/**
 * @brief Symtable hash func implemeted as sdbm
 * @param key key to be hashed
 * @return Hash for our item
 */
static size_t symtableHashFunction(symtableKeyT key) {
    size_t hash = 0;
    const unsigned char *ptr;
    for (ptr = (const unsigned char *)key; *ptr != '\0'; ptr++) {
        hash = (65599 * hash) + *ptr;
    }
    return hash;
}

/**
 * @brief Checks if used slots take more than 60% of space
 * @param table
 * @return True if we need to rehash false otherwise
 */
static bool checkForRehash(Symtable const *table) {
    return (table->usedSlots * 10 >= table->capacity * REHASH_LIMIT);
}

/**
 * @brief Funciton for rehasimg the item array with double capacity
 * @param table
 * @return True if succes
 */
static bool rehash(Symtable *table) {
    if (table == NULL) {
        return false;
    }
    size_t newCapacity = table->capacity * 2;
    symtableItem *newItems = calloc(newCapacity, sizeof(symtableItem));
    if (newItems == NULL) {
        return false;
    }
    for (size_t i = 0; i < newCapacity; i++) {
        newItems[i].state = SLOT_EMPTY;
    }
    for (size_t i = 0; i < table->capacity; i++) {
        if (table->items[i].state == SLOT_OCCUPIED) {
            size_t idx = symtableHashFunction(table->items[i].key) % newCapacity;
            while (newItems[idx].state != SLOT_EMPTY) {
                idx = (idx + 1) % newCapacity;
            }
            newItems[idx] = table->items[i];
            newItems[idx].state = SLOT_OCCUPIED;
        }
    }
    free(table->items);
    table->items = newItems;
    table->capacity = newCapacity;
    table->usedSlots = table->size;
    return true;
}

/**
 * @brief Initilizes SymtableItem at idx with given key
 * @param symtable target symtable
 * @param idx index where you want to place your item
 * @param key string which you want to add as a key
 * @return Pointer to the new initalized item or NULL in case of error of malloc
 */
static symtableItem *itemInit(Symtable *symtable, size_t idx, const char *key) {
    // TODO nevim jeste co presne mi posle syntax analyzator takze zatim tam
    // hodim jenom klic
    if (symtable == NULL || key == NULL || symtable->items[idx].state == SLOT_OCCUPIED) {
        return NULL;
    }
    size_t len = strlen(key) + 1;
    symtable->items[idx].key = malloc(len);
    if (symtable->items[idx].key == NULL) {
        return NULL;
    }
    memcpy(symtable->items[idx].key, key, len);
    symtable->items[idx].state = SLOT_OCCUPIED;
    symtable->items[idx].data.var.used = false;
    symtable->size++;
    symtable->usedSlots++;
    if (checkForRehash(symtable)) {
        rehash(symtable);
        return symtableFind(symtable, key);
    }
    return &symtable->items[idx];
}

symtableItem *symtableLookupAdd(Symtable *table, symtableKeyT key) {
    if (table == NULL || key == NULL) {
        return NULL;
    }
    size_t idx = symtableHashFunction(key) % table->capacity;
    size_t temp = idx;
    size_t auxil = table->capacity + 2;
    do {
        if (table->items[temp].state == SLOT_OCCUPIED && strcmp(table->items[temp].key, key) == 0) {
            return &table->items[temp];
        }
        if (table->items[temp].state == SLOT_EMPTY) {
            if (auxil != table->capacity + 2) {
                return itemInit(table, auxil, key);
            }
            return itemInit(table, temp, key);
        }
        if (table->items[temp].state == SLOT_DELETED && auxil == table->capacity + 2) {
            auxil = temp;
        }
        temp = (temp + 1) % table->capacity;
    } while (temp != idx);
    if (auxil != table->capacity + 2) {
        return itemInit(table, auxil, key);
    }
    return NULL;
}

symtableItem *symtableFind(Symtable *table, symtableKeyT key) {
    if (table == NULL || key == NULL) {
        return NULL;
    }
    size_t idx = symtableHashFunction(key) % table->capacity;
    size_t temp = idx;
    do {
        if (table->items[temp].state == SLOT_EMPTY) {
            return NULL;
        }
        if (table->items[temp].state == SLOT_OCCUPIED && strcmp(table->items[temp].key, key) == 0) {
            return &table->items[temp];
        }
        temp = (temp + 1) % table->capacity;
    } while (temp != idx);
    return NULL;
}

void symtablePrint(Symtable const *table) {
    for (size_t i = 0; i < table->capacity; i++) {
        if (table->items[i].state == SLOT_OCCUPIED) {
            printf("%zu:    %s\n", i, table->items[i].key);
        }
        if (table->items[i].state == SLOT_EMPTY) {
            printf("%zu:     \n", i);
        }
    }
}
