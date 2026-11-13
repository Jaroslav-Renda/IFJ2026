/**
 * @file    error.c
 * @brief   Implementation of global error handeling
 * @author  Jaroslav Renda (xrendaj00)
 * @details project_IFJ2026
 */

#include "error.h"
#include <stdio.h>

void debugPrintError(char const *str) { (void)fprintf(stderr, "[DEBUG]: %s", str); }
