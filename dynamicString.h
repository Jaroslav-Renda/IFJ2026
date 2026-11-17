/**
 * @file    dynamicString.h
 * @author  Jakub Zenata (xzenatj00)
 * @brief   header file dynamic string
 * @date    2026-11-16
 * @details project_IFJ2026
 */

#ifndef DYNAMIC_STRING_H
#define DYNAMIC_STRING_H

/**
 * @brief struct representing dynamic string
 */

typedef struct TDynamicString {
    unsigned long length;
    unsigned long allocSize;
    char *str;
} DynamicString;

/**
 * @brief Function creates new string
 * @param dynamicStr dynamic string to be initialized
 * @return int Returns 0 on succesfull init, or 1 in case of failure
 */
int strInit(DynamicString *dynamicStr);
/**
 * @brief Function frees allocated memory of string
 * @param dynamicStr dynamic string to be freed
 */
void strFree(DynamicString *dynamicStr);

/**
 * @brief Function adds character to end of string
 * @param dynamicStr dynamic string to which char is added
 * @param charAdded char added to end of string
 * @return int Returns 0 on succesfull add, or 1 in case of failure
 */
int strAddChar(DynamicString *dynamicStr, char charAdded);

/**
 * @brief Function clears string
 * @param dynamicStr dynamic string which should be cleared
 */
void strClear(DynamicString *dynamicStr);
/**
 * @brief Function compares dynamic string with a constant string
 * @param str1 dynamic string which should be compared
 * @param str2 const string which should be compared
 *
 * @return int Returns result of strcmp
 */
int strCompare(DynamicString *str1, const char *str2);

#endif
