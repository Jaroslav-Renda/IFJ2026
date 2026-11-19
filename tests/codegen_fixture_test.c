/**
 * @file    codegen_fixture_test.c
 * @brief   Runs semantical analysis and code generation over a fixed AST
 * @author  Vojtech Urich (xurich00)
 * @date    2026-11-18
 * @details project: IFJ2026
 *
 * Standalone driver, not part of `make build`: the Makefile compiles the root
 * directory only. Build and run it with tests/run.sh.
 *
 * Pipeline under test: fixture AST -> semantAnalyse() -> codegenStart().
 * The lexer and the recursive-descent parser are deliberately absent.
 *
 * codegenStart() writes IFJcode26 to stdout and reports unimplemented paths
 * through errorInternal(), which only records a return code and keeps running,
 * so the exit status of this driver is derived from errorReturnValue().
 *
 * AI-assisted include/registry additions by Kimi. Prompt: "make new and more
 * tests like how they are in the tests folder, for now only for the codegen".
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../codegen.h"
#include "../error.h"
#include "../semantAnalys.h"
#include "arith_fixture.h"
#include "control_fixture.h"
#include "factorial_fixture.h"
#include "functions_fixture.h"
#include "literals_fixture.h"
#include "looplocals_fixture.h"
#include "numeric_fixture.h"
#include "ordbounds_fixture.h"
#include "readers_fixture.h"
#include "scopes_fixture.h"
#include "strings_fixture.h"
#include "substring_fixture.h"
#include "widening_fixture.h"

/// @brief Writes a line to stderr and turns a write failure into a hard exit.
static void report(const char *text) {
    if (fputs(text, stderr) == EOF) {
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Registry of the available fixtures.
 *
 * The first entry is the default when the driver is run without an argument,
 * so the historical `run.sh` invocation keeps working.
 */
typedef struct {
    const char *name;
    ASTNode *(*build)(void);
    const char *description;
} FixtureEntry;

/* AI-assisted additions by Kimi. Prompt: "make new and more tests like how they are in the
 * tests folder, for now only for the codegen". */
static const FixtureEntry fixtures[] = {
    {"factorial", factorialFixtureBuild, "official basic/factorial.scala"},
    {"strings", stringsFixtureBuild, "official basic/strings.scala"},
    {"arith", arithFixtureBuild, "synthetic: int/double arithmetic, unary ops"},
    {"control", controlFixtureBuild, "synthetic: nested if/else, early return from loop"},
    {"numeric", numericFixtureBuild, "numeric operators, conversions, all relations"},
    {"functions", functionsFixtureBuild, "multi-parameter calls, zero args, bare return"},
    {"literals", literalsFixtureBuild, "exact string escaping and empty literals"},
    {"scopes", scopesFixtureBuild, "nested and sibling variable shadowing"},
    {"looplocals", looplocalsFixtureBuild, "regression: declarations in repeated loop bodies"},
    {"widening", wideningFixtureBuild, "regression: Int-to-Double at value boundaries"},
    {"readers", readersFixtureBuild, "valid, malformed, and EOF input"},
    {"substring", substringFixtureBuild, "subStr exclusive end-index semantics"},
    {"ordbounds", ordboundsFixtureBuild, "ord returns -1 outside string bounds"},
};

enum {
    FIXTURE_COUNT = sizeof(fixtures) / sizeof(fixtures[0]),
};

/// @brief Prints the fixture list (driver argument "list").
static void listFixtures(void) {
    for (int index = 0; index < FIXTURE_COUNT; index++) {
        if (fprintf(stderr, "%-10s %s\n", fixtures[index].name, fixtures[index].description) < 0) {
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[]) {
    const FixtureEntry *chosen = &fixtures[0];

    if (argc > 2) {
        if (fprintf(stderr, "usage: %s [fixture|list]  (IFJcode26 goes to stdout)\n", argv[0]) <
            0) {
            exit(EXIT_FAILURE);
        }
        return EXIT_FAILURE;
    }
    if (argc == 2) {
        if (strcmp(argv[1], "list") == 0) {
            listFixtures();
            return EXIT_SUCCESS;
        }
        chosen = NULL;
        for (int index = 0; index < FIXTURE_COUNT; index++) {
            if (strcmp(argv[1], fixtures[index].name) == 0) {
                chosen = &fixtures[index];
                break;
            }
        }
        if (chosen == NULL) {
            if (fprintf(stderr, "unknown fixture '%s'; known fixtures:\n", argv[1]) < 0) {
                exit(EXIT_FAILURE);
            }
            listFixtures();
            return EXIT_FAILURE;
        }
    }

    ASTNode *root = chosen->build();
    if (root == NULL) {
        report("fixture: out of memory while building the AST\n");
        return EXIT_FAILURE;
    }

    uint8_t semErr = semantAnalyse(root);
    if (semErr != OK) {
        if (fprintf(stderr, "semantAnalyse: rejected the AST with error %u\n", semErr) < 0) {
            exit(EXIT_FAILURE);
        }
        deleteNode(root);
        return EXIT_FAILURE;
    }
    report("semantAnalyse: OK\n");

    codegenStart(root);

    int compilerErr = errorReturnValue();
    if (compilerErr != 0) {
        if (fprintf(stderr, "codegen: reported internal error, exit code %d\n", compilerErr) < 0) {
            exit(EXIT_FAILURE);
        }
    }

    if (fflush(stdout) == EOF) {
        report("codegen: failed to flush the IFJcode26 output\n");
        deleteNode(root);
        return EXIT_FAILURE;
    }

    deleteNode(root);
    return (compilerErr == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
