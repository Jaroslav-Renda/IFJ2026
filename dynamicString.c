/**
 * @file    dynamicString.c
 * @author  Jakub Zenata (xzenatj00)
 * @brief   Source file of dynamic string
 * @date    2026-11-16
 * @details project_IFJ2026
 */
#include "dynamicString.h"
#include <stdlib.h>
#include <string.h>

#define DEFAULT_STRING_SIZE 16

int strInit(DynamicString *dynamicStr) {
    dynamicStr->str = (char *)malloc(DEFAULT_STRING_SIZE);
    if (dynamicStr->str == NULL) {
        return 1;
    }
    dynamicStr->str[0] = '\0';
    dynamicStr->length = 0;
    dynamicStr->allocSize = DEFAULT_STRING_SIZE;
    return 0;
}
void strFree(DynamicString *dynamicStr) {
    free(dynamicStr->str);
    dynamicStr->str = NULL;
    dynamicStr->length = 0;
    dynamicStr->allocSize = 0;
}
int strAddChar(DynamicString *dynamicStr, char charAdded) {
    if (dynamicStr->length + 1 >= dynamicStr->allocSize) {
        unsigned long newSize = dynamicStr->allocSize * 2;
        char *temp = (char *)realloc(dynamicStr->str, newSize);
        if (temp == NULL) {
            return 1;
        }
        dynamicStr->str = temp;
        dynamicStr->allocSize = newSize;
    }
    dynamicStr->str[dynamicStr->length] = charAdded;
    dynamicStr->length++;
    dynamicStr->str[dynamicStr->length] = '\0';
    return 0;
}
void strClear(DynamicString *dynamicStr) {
    dynamicStr->str[0] = '\0';
    dynamicStr->length = 0;
}

int strCompare(DynamicString *dynamicStr1, const char *str2) {
    return strcmp(dynamicStr1->str, str2);
}
