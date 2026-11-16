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

typedef struct dynamicString {
    unsigned long length;
    unsigned long allocSize;
    char *str;
} tDynamicString;

/**
 * @brief Function creates new string
 * @param s dynamic string to be initialized
 * @return int Returns 0 on succesfull init, or 1 in case of failure
 */
int strInit(tDynamicString *s);
/**
 * @brief Function frees allocated memory of string
 * @param s dynamic string to be freed
 */
void strFree(tDynamicString *s);

/**
 * @brief Function adds character to end of string
 * @param s dynamic string to which char is added
 * @param c char added to end of string
 * @return int Returns 0 on succesfull add, or 1 in case of failure
 */
int strAddChar(tDynamicString *s, char c);

/**
 * @brief Function clears string
 * @param s dynamic string which should be cleared
 */
void strClear(tDynamicString *s);
/**
 * @brief Function compares dynamic string with a constant string
 * @param s1 dynamic string which should be compared
 * @param s2 const string which should be compared
 *
 * @return int Returns result of strcmp
 */
int strCompare(tDynamicString *s1, const char *s2);

#endif
