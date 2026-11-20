/**
 * @file    main.c
 * @brief   Main file for compiler startup
 * @author  Jaroslav Renda (xrendaj00)
 * @details project_IFJ2026
 */

#include "error.h"
#include "lexer.h"
#include "parser.h"
#include <stdlib.h>

int main() {
    currentToken cur;
    parseProg(&cur);
    return errorReturnValue();
}
