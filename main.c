    /**
     * @file    main.c
     * @brief   Main file for compiler startup
     * @author  Jaroslav Renda (xrendaj00)
     * @details project_IFJ2026
     */

    #include "error.h"
    #include "lexer.h"
    #include "parser.h"

    int main(void) {
        currentToken cur;

        lexerStateInit();
        if (!parseProg(&cur) && errorReturnValue() == OK) {
            errorParser(cur.token.line, "failed to parse input program");
        }

        return errorReturnValue();
    }
