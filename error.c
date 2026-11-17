/**
 * @file    error.c
 * @brief   Implementation of global error handling
 * @author  Petr Mácha (xmachap00)
 * @details project_IFJ2026
 */

#include "error.h"

#include <stdio.h>

// Value that will be returned by the program
static int g_returnNum = 0;

void errorLexer(int lineNumber, const char *str) {
    if (g_returnNum == 0) {
        g_returnNum = ERR_LEXER;
    }

    (void)fprintf(stderr, "line %i: error in lexical analysis: %s\n", lineNumber, str);
}

void errorParser(int lineNumber, const char *str) {
    if (g_returnNum == 0) {
        g_returnNum = ERR_PARSER;
    }
    (void)fprintf(stderr, "line %i: error in syntactic analysis: %s\n", lineNumber, str);
}

void errorSemUndefined(int lineNumber, const char *str) {
    if (g_returnNum == 0) {
        g_returnNum = ERR_SEM_UNDEFINED;
    }
    (void)fprintf(stderr, "line %i: error in semantic analysis: %s\n", lineNumber, str);
}

void errorSemRedefinition(int lineNumber, const char *str) {
    if (g_returnNum == 0) {
        g_returnNum = ERR_SEM_REDEFINITION;
    }
    (void)fprintf(stderr, "line %i: error in semantic analysis: %s\n", lineNumber, str);
}

void errorSemArguments(int lineNumber, const char *str) {
    if (g_returnNum == 0) {
        g_returnNum = ERR_SEM_ARGUMENTS;
    }
    (void)fprintf(stderr, "line %i: error in semantic analysis: %s\n", lineNumber, str);
}

void errorSemType(int lineNumber, const char *str) {
    if (g_returnNum == 0) {
        g_returnNum = ERR_SEM_TYPE;
    }
    (void)fprintf(stderr, "line %i: error in semantic analysis: %s\n", lineNumber, str);
}

void errorSemImmutable(int lineNumber, const char *str) {
    if (g_returnNum == 0) {
        g_returnNum = ERR_SEM_IMMUTABLE;
    }
    (void)fprintf(stderr, "line %i: error in semantic analysis: %s\n", lineNumber, str);
}

void errorSemOther(int lineNumber, const char *str) {
    if (g_returnNum == 0) {
        g_returnNum = ERR_SEM_OTHER;
    }
    (void)fprintf(stderr, "line %i: error in semantic analysis: %s\n", lineNumber, str);
}

void errorInternal(int lineNumber, const char *str) {
    if (g_returnNum == 0) {
        g_returnNum = ERR_INTERNAL;
    }
    (void)fprintf(stderr, "line %i: internal error: %s\n", lineNumber, str);
}

void errorDebug(const char *str) { (void)fprintf(stderr, "debug: %s", str); }

int errorReturnValue() { return g_returnNum; }
