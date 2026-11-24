/**
 * @file    codegen.c
 * @brief   IFJcode26 generator module
 * @author  Petr Mácha (xmachap00)
 * @details project_IFJ2026
 */

#define OUTPUT(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...)                                                                            \
    errorInternal(0, "codegen: " fmt, ##__VA_ARGS__);                                              \
    exit(1)

#include "codegen.h"

#include "ast.h"
#include "error.h"

#include <stdio.h>
#include <string.h>

// True while generating the @main function body
static bool codegenGeneratingMain = false;

// Frame prefix for variable operands: "GF@" inside main, "LF@" everywhere else
static const char *codegenFramePrefix(void) { return codegenGeneratingMain ? "GF@" : "LF@"; }

static void codegenEpilogue(void) {
    if (codegenGeneratingMain) {
        OUTPUT("EXIT int@0\n");
    } else {
        OUTPUT("POPFRAME\n");
        OUTPUT("RETURN\n");
    }
}

// Sequential id for unique temp variable/label names
static int codegenUniqueId(void) {
    static int id = 0;
    return id++;
}

static void codegenTempName(char *buffer, size_t size) {
    snprintf(buffer, size, "ifj$tmp$%i", codegenUniqueId());
}

static void codegenLabelName(const char *prefix, char *buffer, size_t size) {
    snprintf(buffer, size, "%s$%i", prefix, codegenUniqueId());
}

void codegenLiteral(ASTNode *node) {
    if (node->type != NODE_LITERAL) {
        ERROR("Called codegenLiteral with incorrect node type");
    }

    switch (node->data.literal.litType) {
    case TYPE_DOUBLE:
        OUTPUT("PUSHS float@%a\n", node->data.literal.value.doubleVal);
        break;
    case TYPE_INT:
        OUTPUT("PUSHS int@%li\n", node->data.literal.value.intVal);
        break;
    case TYPE_STRING:
        OUTPUT("PUSHS string@");

        // Print out the string by character for ascii escaping
        int charCursor = 0;
        while (node->data.literal.value.strVal[charCursor] != '\0') {
            // escape whitespace, # and \ as \XXX (decimal ASCII code)
            int character = (unsigned char)node->data.literal.value.strVal[charCursor];
            if (character <= 32 || character == 35 || character == 92) {
                OUTPUT("\\%.3i", character);
            } else {
                OUTPUT("%c", character);
            }
            charCursor++;
        }
        OUTPUT("\n");
        break;
    case TYPE_UNKNOWN:
    case TYPE_UNIT:
    case TYPE_ANY:
    case TYPE_BOOL:
        ERROR("fix: hit unknown/unit/any in codegenInternal");
        break;
    }
}

void codegenVarRef(ASTNode *node) {
    if (node->type != NODE_VAR_REF) {
        ERROR("Called codegenLiteral with incorrect node type");
    }

    OUTPUT("PUSHS %s%s\n", codegenFramePrefix(), node->data.varRef.mangledName);
}

void codegenUnaryOp(ASTNode *node) {
    if (node->type != NODE_UNARY_OP) {
        ERROR("Called codegenUnaryOp with incorrect node type");
    }

    // the operand value ends up on the stack
    switch (node->data.unaryOp.op) {
    case UOP_PLUS:
        codegenPostOrderExpr(node->data.unaryOp.operand);
        // identity - nothing to do
        break;
    case UOP_MINUS:
        // 0 - x: SUBS computes second-from-top minus top, so the zero goes first
        if (node->semanticType == TYPE_INT) {
            OUTPUT("PUSHS int@0\n");
        } else {
            OUTPUT("PUSHS float@0x0p+0\n");
        }
        codegenPostOrderExpr(node->data.unaryOp.operand);
        OUTPUT("SUBS\n");
        break;
    }
}

// Concatenates the two strings on top of the stack (right on top) and pushes the result
static void codegenConcat(void) {
    const char *frame = codegenFramePrefix();
    char tmp1[64];
    char tmp2[64];
    codegenTempName(tmp1, sizeof(tmp1));
    codegenTempName(tmp2, sizeof(tmp2));

    OUTPUT("DEFVAR %s%s\n", frame, tmp1);
    OUTPUT("DEFVAR %s%s\n", frame, tmp2);
    OUTPUT("POPS %s%s\n", frame, tmp2);
    OUTPUT("POPS %s%s\n", frame, tmp1);
    OUTPUT("CONCAT %s%s %s%s %s%s\n", frame, tmp1, frame, tmp1, frame, tmp2);
    OUTPUT("PUSHS %s%s\n", frame, tmp1);
}

void codegenBinaryOp(ASTNode *node) {
    if (node->type != NODE_BIN_OP) {
        ERROR("Called codegenLiteral with incorrect node type");
    }

    switch (node->data.binOp.op) {
    case OP_ADD:
        if (node->semanticType == TYPE_STRING) {
            codegenConcat();
        } else {
            OUTPUT("ADDS\n");
        }
        break;
    case OP_SUB:
        OUTPUT("SUBS\n");
        break;
    case OP_MUL:
        OUTPUT("MULS\n");
        break;
    case OP_DIV:
        // Int/Int is integer division, anything involving Double is float division
        // (int operands were already converted by the INT2FLOATS emitted for convertToDouble)
        if (node->semanticType == TYPE_INT) {
            OUTPUT("IDIVS\n");
        } else {
            OUTPUT("DIVS\n");
        }
        break;
    case OP_EQ:
        OUTPUT("EQS\n");
        break;
    case OP_NEQ:
        OUTPUT("EQS\n");
        OUTPUT("NOTS\n");
        break;
    // TODO: Optimize comparing operands
    case OP_LT:
        OUTPUT("LTS\n");
        break;
    case OP_GT:
        OUTPUT("GTS\n");
        break;
    case OP_LTE:
        // inverted
        OUTPUT("GTS\n");
        OUTPUT("NOTS\n");
        break;
    case OP_GTE:
        // inverted
        OUTPUT("LTS\n");
        OUTPUT("NOTS\n");
        break;
    }
}

void codegenPostOrderExpr(ASTNode *node) {
    switch (node->type) {
    case NODE_LITERAL:
        codegenLiteral(node);
        break;
    case NODE_VAR_REF:
        codegenVarRef(node);
        break;
    case NODE_UNARY_OP:
        codegenUnaryOp(node);
        break;
    case NODE_BIN_OP:
        // operand widening is emitted by the recursive calls' exit check below
        codegenPostOrderExpr(node->data.binOp.left);
        codegenPostOrderExpr(node->data.binOp.right);

        codegenBinaryOp(node);
        break;
    case NODE_FUNC_CALL:
        codegenFuncCall(node);
        break;
    default:
        break;
    }

    // Single widening point: semantic analysis sets convertToDouble on every
    // expression root that feeds a Double context (declaration init, assignment,
    // call argument, explicit return, implicit tail expression), and the
    // recursive binOp calls route operand conversions through this same check.
    if (node->convertToDouble) {
        OUTPUT("INT2FLOATS\n");
    }
}

// Defines every non-parameter variable of the function at once at the top of
// its body; declarations then only evaluate initializers and store them.
// The vars list starts with the parameters, which codegenParam defines itself.
static void codegenDefineFuncVars(ASTNode *node) {
    const char *frame = codegenFramePrefix();

    size_t paramCount = 0;
    for (ASTNode *param = node->data.funcDef.params; param != NULL; param = param->next) {
        paramCount++;
    }

    FuncVarEntry *var = node->data.funcDef.vars;
    while (paramCount-- > 0 && var != NULL) {
        var = var->next;
    }

    for (; var != NULL; var = var->next) {
        OUTPUT("DEFVAR %s%s\n", frame, var->mangledName);
    }
}

void codegenVarDecl(ASTNode *node) {
    if (node->type != NODE_VAR_DECL) {
        ERROR("Called codegenVarDecl with incorrect node type");
    }

    // DEFVAR is already emitted by codegenDefineFuncVars for all variables
    codegenPostOrderExpr(node->data.varDecl.initExpr);

    OUTPUT("POPS %s%s\n", codegenFramePrefix(), node->data.varDecl.mangledName);
}

void codegenParam(ASTNode *node) {
    if (node->type != NODE_PARAM) {
        ERROR("Called codegenParam with incorrect node type");
    }

    OUTPUT("DEFVAR %s%s\n", codegenFramePrefix(), node->data.param.mangledName);
    OUTPUT("POPS %s%s\n", codegenFramePrefix(), node->data.param.mangledName);
}

void codegenAssign(ASTNode *node) {
    if (node->type != NODE_ASSIGN) {
        ERROR("Called codegenAssign with incorrect node type");
    }

    codegenPostOrderExpr(node->data.assign.expr);

    OUTPUT("POPS %s%s\n", codegenFramePrefix(), node->data.assign.mangledName);
}

void codegenIf(ASTNode *node) {
    if (node->type != NODE_IF) {
        ERROR("Called codegenIf with incorrect node type");
    }

    char ifEndName[64];
    codegenLabelName("if$end", ifEndName, sizeof(ifEndName));

    char ifFalseName[64];
    codegenLabelName("if$false", ifFalseName, sizeof(ifFalseName));

    codegenPostOrderExpr(node->data.ifStmt.condition);
    OUTPUT("PUSHS bool@true\n");
    OUTPUT("JUMPIFNEQS %s\n", ifFalseName);
    // true block
    codegenWalk(node->data.ifStmt.thenBlock, false);
    OUTPUT("JUMP %s\n", ifEndName);
    OUTPUT("LABEL %s\n", ifFalseName);
    // false(else) block
    codegenWalk(node->data.ifStmt.elseBlock, false);
    OUTPUT("LABEL %s\n", ifEndName);
}

void codegenWhile(ASTNode *node) {
    if (node->type != NODE_WHILE) {
        ERROR("Called codegenWhile with incorrect node type");
    }

    // loop head: evaluate the condition, leave the body when it turns out false
    char whileStartName[64];
    codegenLabelName("while$start", whileStartName, sizeof(whileStartName));
    char whileEndName[64];
    codegenLabelName("while$end", whileEndName, sizeof(whileEndName));

    OUTPUT("LABEL %s\n", whileStartName);
    codegenPostOrderExpr(node->data.whileStmt.condition);
    OUTPUT("PUSHS bool@true\n");
    OUTPUT("JUMPIFNEQS %s\n", whileEndName);
    codegenWalk(node->data.whileStmt.bodyBlock, false);
    OUTPUT("JUMP %s\n", whileStartName);
    OUTPUT("LABEL %s\n", whileEndName);
}

static bool codegenIsBuiltin(const char *name) {
    static const char *builtins[] = {"print", "readLine", "readInt", "readDouble", "toStr", "toInt",
        "strLength", "subStr", "ord", "chr"};

    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        if (strcmp(name, builtins[i]) == 0) {
            return true;
        }
    }
    return false;
}

// Pushes arguments onto the stack in source order (last argument ends up on top)
static void codegenPushArgsForward(ASTNode *arg) {
    if (arg == NULL) {
        return;
    }

    codegenPostOrderExpr(arg);
    codegenPushArgsForward(arg->next);
}

// Pushes arguments onto the stack in reverse order (first argument ends up on top),
// matching codegenParam which pops the parameters in their declaration order
static void codegenPushArgsReversed(ASTNode *arg) {
    if (arg == NULL) {
        return;
    }

    codegenPushArgsReversed(arg->next);
    codegenPostOrderExpr(arg);
}

// Generates code for a call of a built-in function. Arguments are on the stack in
// source order (except print, which is emitted directly as WRITE); the return value
// (if any) is left on the top of the stack.
static void codegenBuiltinCall(ASTNode *node) {
    const char *name = node->data.funcCall.funcName;
    ASTNode *arg = node->data.funcCall.args;
    const char *frame = codegenFramePrefix();
    char tmp1[64];
    char tmp2[64];
    char label[64];

    // Stack-based builtins take their arguments from the data stack in source order
    // (read* take no arguments, print is emitted directly below)
    if (strcmp(name, "print") != 0) {
        codegenPushArgsForward(node->data.funcCall.args);
    }

    if (strcmp(name, "print") == 0) {
        codegenTempName(tmp1, sizeof(tmp1));
        OUTPUT("DEFVAR %s%s\n", frame, tmp1);
        codegenPostOrderExpr(arg);
        OUTPUT("POPS %s%s\n", frame, tmp1);
        OUTPUT("WRITE %s%s\n", frame, tmp1);
    } else if (strcmp(name, "readLine") == 0) {
        codegenTempName(tmp1, sizeof(tmp1));
        OUTPUT("DEFVAR %s%s\n", frame, tmp1);
        OUTPUT("READ %s%s string\n", frame, tmp1);
        OUTPUT("PUSHS %s%s\n", frame, tmp1);
    } else if (strcmp(name, "readInt") == 0) {
        // READ leaves the destination nil on an invalid line or at EOF; IFJ26
        // readInt must return 0 in those cases (ifj26.scala reference).
        // nil is not a literal operand, so the type is probed with TYPE instead.
        codegenTempName(tmp1, sizeof(tmp1));
        codegenTempName(tmp2, sizeof(tmp2));
        codegenLabelName("ifj$read$nil", label, sizeof(label));
        char labelDone[64];
        codegenLabelName("ifj$read$done", labelDone, sizeof(labelDone));
        OUTPUT("DEFVAR %s%s\n", frame, tmp1);
        OUTPUT("DEFVAR %s%s\n", frame, tmp2);
        OUTPUT("READ %s%s int\n", frame, tmp1);
        OUTPUT("TYPE %s%s %s%s\n", frame, tmp2, frame, tmp1);
        OUTPUT("JUMPIFEQ %s %s%s string@nil\n", label, frame, tmp2);
        OUTPUT("PUSHS %s%s\n", frame, tmp1);
        OUTPUT("JUMP %s\n", labelDone);
        OUTPUT("LABEL %s\n", label);
        OUTPUT("PUSHS int@0\n");
        OUTPUT("LABEL %s\n", labelDone);
    } else if (strcmp(name, "readDouble") == 0) {
        // Same nil guard as readInt: an invalid line or EOF must yield 0.0
        // instead of an uninitialized nil (ifj26.scala reference).
        codegenTempName(tmp1, sizeof(tmp1));
        codegenTempName(tmp2, sizeof(tmp2));
        codegenLabelName("ifj$read$nil", label, sizeof(label));
        char labelDone[64];
        codegenLabelName("ifj$read$done", labelDone, sizeof(labelDone));
        OUTPUT("DEFVAR %s%s\n", frame, tmp1);
        OUTPUT("DEFVAR %s%s\n", frame, tmp2);
        OUTPUT("READ %s%s float\n", frame, tmp1);
        OUTPUT("TYPE %s%s %s%s\n", frame, tmp2, frame, tmp1);
        OUTPUT("JUMPIFEQ %s %s%s string@nil\n", label, frame, tmp2);
        OUTPUT("PUSHS %s%s\n", frame, tmp1);
        OUTPUT("JUMP %s\n", labelDone);
        OUTPUT("LABEL %s\n", label);
        OUTPUT("PUSHS float@0x0p+0\n");
        OUTPUT("LABEL %s\n", labelDone);
    } else if (strcmp(name, "toStr") == 0) {
        // Result of the argument stays on the stack; convert by its static type
        switch (arg->semanticType) {
        case TYPE_STRING:
            // already a string, nothing to do
            break;
        case TYPE_INT:
            OUTPUT("INT2STRS\n");
            break;
        case TYPE_DOUBLE:
            OUTPUT("FLOAT2STRS\n");
            break;
        case TYPE_BOOL: {
            codegenTempName(tmp1, sizeof(tmp1));
            codegenLabelName("ifj$str$true", label, sizeof(label));
            char labelEnd[64];
            codegenLabelName("ifj$str$end", labelEnd, sizeof(labelEnd));
            OUTPUT("DEFVAR %s%s\n", frame, tmp1);
            OUTPUT("POPS %s%s\n", frame, tmp1);
            OUTPUT("JUMPIFEQ %s %s%s bool@true\n", label, frame, tmp1);
            OUTPUT("MOVE %s%s string@false\n", frame, tmp1);
            OUTPUT("JUMP %s\n", labelEnd);
            OUTPUT("LABEL %s\n", label);
            OUTPUT("MOVE %s%s string@true\n", frame, tmp1);
            OUTPUT("LABEL %s\n", labelEnd);
            OUTPUT("PUSHS %s%s\n", frame, tmp1);
            break;
        }
        default:
            ERROR("toStr: unsupported argument type");
            break;
        }
    } else if (strcmp(name, "toInt") == 0) {
        // toInt(Double): Int - truncate the fractional part
        OUTPUT("FLOAT2INTS\n");
    } else if (strcmp(name, "strLength") == 0) {
        codegenTempName(tmp1, sizeof(tmp1));
        codegenTempName(tmp2, sizeof(tmp2));
        OUTPUT("DEFVAR %s%s\n", frame, tmp1);
        OUTPUT("DEFVAR %s%s\n", frame, tmp2);
        OUTPUT("POPS %s%s\n", frame, tmp1);
        OUTPUT("STRLEN %s%s %s%s\n", frame, tmp2, frame, tmp1);
        OUTPUT("PUSHS %s%s\n", frame, tmp2);
    } else if (strcmp(name, "subStr") == 0) {
        // substr(str, start, end) - concatenate GETCHAR of str[start], ..., str[end - 1];
        // the third argument is the exclusive end index (ifj26.scala reference), so an
        // empty result falls out of the loop naturally when start >= end
        codegenTempName(tmp1, sizeof(tmp1));
        codegenTempName(tmp2, sizeof(tmp2));
        char labelLoop[64];
        codegenLabelName("ifj$substr$loop", labelLoop, sizeof(labelLoop));
        char labelEnd[64];
        codegenLabelName("ifj$substr$end", labelEnd, sizeof(labelEnd));
        // stack: str, start, end (end on top)
        static const char *substrSuffixes[] = {"$s", "$i", "$n", "$res", "$c"};
        for (size_t suffixIndex = 0;
            suffixIndex < sizeof(substrSuffixes) / sizeof(substrSuffixes[0]); suffixIndex++) {
            char defName[80];
            snprintf(defName, sizeof(defName), "%s%s", tmp1, substrSuffixes[suffixIndex]);
            OUTPUT("DEFVAR %s%s\n", frame, defName);
        }
        OUTPUT("DEFVAR %s%s\n", frame, tmp2); // relation scratch
        OUTPUT("POPS %s%s$n\n", frame, tmp1);
        OUTPUT("POPS %s%s$i\n", frame, tmp1);
        OUTPUT("POPS %s%s$s\n", frame, tmp1);
        OUTPUT("MOVE %s%s$res string@\n", frame, tmp1);
        OUTPUT("LABEL %s\n", labelLoop);
        // loop while the read index $i has not reached the exclusive end $n
        OUTPUT("LT %s%s %s%s$i %s%s$n\n", frame, tmp2, frame, tmp1, frame, tmp1);
        OUTPUT("JUMPIFEQ %s %s%s bool@false\n", labelEnd, frame, tmp2);
        OUTPUT("GETCHAR %s%s$c %s%s$s %s%s$i\n", frame, tmp1, frame, tmp1, frame, tmp1);
        OUTPUT("CONCAT %s%s$res %s%s$res %s%s$c\n", frame, tmp1, frame, tmp1, frame, tmp1);
        OUTPUT("ADD %s%s$i %s%s$i int@1\n", frame, tmp1, frame, tmp1);
        OUTPUT("JUMP %s\n", labelLoop);
        OUTPUT("LABEL %s\n", labelEnd);
        OUTPUT("PUSHS %s%s$res\n", frame, tmp1);
    } else if (strcmp(name, "ord") == 0) {
        // ord(str, i): stack: str, i (i on top). Spec: returns -1 when the string
        // is empty or the index is outside the string; never interpreter error 58
        codegenTempName(tmp1, sizeof(tmp1));
        static const char *ordSuffixes[] = {"$i", "$s", "$len", "$neg", "$lt", "$res"};
        for (size_t suffixIndex = 0; suffixIndex < sizeof(ordSuffixes) / sizeof(ordSuffixes[0]);
            suffixIndex++) {
            char defName[80];
            snprintf(defName, sizeof(defName), "%s%s", tmp1, ordSuffixes[suffixIndex]);
            OUTPUT("DEFVAR %s%s\n", frame, defName);
        }
        char labelOut[64];
        codegenLabelName("ifj$ord$out", labelOut, sizeof(labelOut));
        char labelDone[64];
        codegenLabelName("ifj$ord$done", labelDone, sizeof(labelDone));
        OUTPUT("POPS %s%s$i\n", frame, tmp1);
        OUTPUT("POPS %s%s$s\n", frame, tmp1);
        OUTPUT("STRLEN %s%s$len %s%s$s\n", frame, tmp1, frame, tmp1);
        // guard 1: index < 0 -> out of range
        OUTPUT("LT %s%s$neg %s%s$i int@0\n", frame, tmp1, frame, tmp1);
        // guard 2: !(index < len) -> out of range; also covers the empty string
        OUTPUT("LT %s%s$lt %s%s$i %s%s$len\n", frame, tmp1, frame, tmp1, frame, tmp1);
        OUTPUT("JUMPIFEQ %s %s%s$neg bool@true\n", labelOut, frame, tmp1);
        OUTPUT("JUMPIFEQ %s %s%s$lt bool@false\n", labelOut, frame, tmp1);
        OUTPUT("STRI2INT %s%s$res %s%s$s %s%s$i\n", frame, tmp1, frame, tmp1, frame, tmp1);
        OUTPUT("JUMP %s\n", labelDone);
        OUTPUT("LABEL %s\n", labelOut);
        OUTPUT("MOVE %s%s$res int@-1\n", frame, tmp1);
        OUTPUT("LABEL %s\n", labelDone);
        OUTPUT("PUSHS %s%s$res\n", frame, tmp1);
    } else if (strcmp(name, "chr") == 0) {
        OUTPUT("INT2CHARS\n");
    } else {
        ERROR("Unknown builtin function");
    }
}

void codegenFuncCall(ASTNode *node) {
    if (node->type != NODE_FUNC_CALL) {
        ERROR("Called codegenFuncCall with incorrect node type");
    }

    if (codegenIsBuiltin(node->data.funcCall.funcName)) {
        codegenBuiltinCall(node);
        return;
    }

    // Push arguments in reverse order (the callee pops them in declaration order),
    // then jump into the function. Its return value (if any) ends on the stack.
    codegenPushArgsReversed(node->data.funcCall.args);
    OUTPUT("CALL %s\n", node->data.funcCall.funcName);
}

void codegenReturn(ASTNode *node) {
    if (node->type != NODE_RETURN) {
        ERROR("Called codegenReturn with incorrect node type");
    }

    // Explicit value: leave it on the stack for the caller to pop. A bare return
    // (Unit function) passes nothing back.
    if (node->data.returnStmt.expr != NULL) {
        codegenPostOrderExpr(node->data.returnStmt.expr);
    }

    codegenEpilogue();
}

void codegenWalk(ASTNode *node, bool inFuncBody) {
    ASTNode *statement = node;
    while (statement != NULL) {
        switch (statement->type) {
        case NODE_BLOCK:
            statement = statement->data.block.firstStmt;
            break;
        case NODE_VAR_DECL:
            codegenVarDecl(statement);
            statement = statement->next;
            break;
        case NODE_ASSIGN:
            codegenAssign(statement);
            statement = statement->next;
            break;
        case NODE_IF:
            codegenIf(statement);
            statement = statement->next;
            break;
        case NODE_WHILE:
            codegenWhile(statement);
            statement = statement->next;
            break;
        case NODE_VAR_REF:
        case NODE_BIN_OP:
        case NODE_UNARY_OP:
        case NODE_FUNC_CALL:
        case NODE_LITERAL:
            codegenPostOrderExpr(statement);
            if (inFuncBody && statement->next == NULL) {
                // last expression of the function body is its implicit return value
                codegenEpilogue();
                return;
            }
            // probably shouldnt even get here
            statement = statement->next;
            break;
        case NODE_RETURN:
            codegenReturn(statement);
            // everything after a return is unreachable
            return;
        default:
            printf("%i", statement->type);
            ERROR("Unexpected type as statement");
        }
    }
}

void codegenFunctionDef(ASTNode *node) {
    if (node->type != NODE_FUNC_DEF) {
        ERROR("Called codegenFunctionDef with incorrect node type");
    }

    // add function name
    bool isMain = node->data.funcDef.isMain;
    codegenGeneratingMain = isMain;

    if (isMain) {
        OUTPUT("LABEL main\n");
    } else {
        OUTPUT("LABEL %s\n", node->data.funcDef.name);
    }

    // main is entered by JUMP, so it keeps its variables in the global frame and
    // pushes no local frame; every other function runs on its own local frame
    if (!isMain) {
        OUTPUT("CREATEFRAME\n");
        OUTPUT("PUSHFRAME\n");
    }

    // create parameters
    ASTNode *parameter = node->data.funcDef.params;
    while (parameter != NULL) {
        codegenParam(parameter);
        parameter = parameter->next;
    }

    // define all variables of the function up front
    codegenDefineFuncVars(node);

    // walk over code
    codegenWalk(node->data.funcDef.body, true);

    // end (dead code when the body already returned)
    codegenEpilogue();
    OUTPUT("\n");
}

void codegenStart(ASTNode *root) {
    if (root->type != NODE_PROGRAM) {
        ERROR("Called codegenStart with incorrect node type");
    }

    // preamble
    OUTPUT(".IFJcode26\n");
    OUTPUT("\n");
    OUTPUT("JUMP main\n");
    OUTPUT("\n");

    // walk over functions
    ASTNode *nextFunction = root->data.program.functions;

    while (nextFunction != NULL) {
        codegenFunctionDef(nextFunction);
        nextFunction = nextFunction->next;
    }
}
