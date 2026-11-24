/**
 * @file    codegen.c
 * @brief   IFJcode26 generator module
 * @author  Petr Mácha (xmachap00)
 * @details project_IFJ2026
 */

#define OUTPUT(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) errorInternal(0, "codegen: " fmt, ##__VA_ARGS__)

#include "codegen.h"

#include "ast.h"
#include "error.h"

#include <stdio.h>
#include <string.h>

// declarations for functions that are used before defined
static void codegenPostOrderExpr(ASTNode *node);
static void codegenFuncCall(ASTNode *node);
static void codegenWalk(ASTNode *node, bool inFuncBody);

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

static void codegenLabelName(const char *prefix, char *buffer, size_t size) {
    snprintf(buffer, size, "%s$%i", prefix, codegenUniqueId());
}

static void codegenLiteral(ASTNode *node) {
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

static void codegenVarRef(ASTNode *node) {
    if (node->type != NODE_VAR_REF) {
        ERROR("Called codegenLiteral with incorrect node type");
    }

    OUTPUT("PUSHS %s%s\n", codegenFramePrefix(), node->data.varRef.mangledName);
}

static void codegenUnaryOp(ASTNode *node) {
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
            OUTPUT("PUSHS int@-0\n");
        } else {
            OUTPUT("PUSHS float@-0x0p+0\n");
        }
        codegenPostOrderExpr(node->data.unaryOp.operand);
        OUTPUT("SUBS\n");
        break;
    }
}

// Concatenates the two strings on top of the stack (right on top) and pushes the result
static void codegenConcat(void) {
    const char *frame = codegenFramePrefix();

    OUTPUT("POPS %s%s\n", frame, "tmp2s");
    OUTPUT("POPS %s%s\n", frame, "tmp1s");
    OUTPUT("CONCAT %s%s %s%s %s%s\n", frame, "tmp1s", frame, "tmp1s", frame, "tmp2s");
    OUTPUT("PUSHS %s%s\n", frame, "tmp1s");
}

static void codegenBinaryOp(ASTNode *node) {
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

static void codegenPostOrderExpr(ASTNode *node) {
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

    if (node->convertToDouble) {
        OUTPUT("INT2FLOATS\n");
    }
}

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

static void codegenVarDecl(ASTNode *node) {
    if (node->type != NODE_VAR_DECL) {
        ERROR("Called codegenVarDecl with incorrect node type");
    }

    codegenPostOrderExpr(node->data.varDecl.initExpr);

    OUTPUT("POPS %s%s\n", codegenFramePrefix(), node->data.varDecl.mangledName);
}

static void codegenParam(ASTNode *node) {
    if (node->type != NODE_PARAM) {
        ERROR("Called codegenParam with incorrect node type");
    }

    OUTPUT("DEFVAR %s%s\n", codegenFramePrefix(), node->data.param.mangledName);
    OUTPUT("POPS %s%s\n", codegenFramePrefix(), node->data.param.mangledName);
}

static void codegenAssign(ASTNode *node) {
    if (node->type != NODE_ASSIGN) {
        ERROR("Called codegenAssign with incorrect node type");
    }

    codegenPostOrderExpr(node->data.assign.expr);

    OUTPUT("POPS %s%s\n", codegenFramePrefix(), node->data.assign.mangledName);
}

static void codegenIf(ASTNode *node) {
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

static void codegenWhile(ASTNode *node) {
    if (node->type != NODE_WHILE) {
        ERROR("Called codegenWhile with incorrect node type");
    }

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

static void codegenPushArgsForward(ASTNode *arg) {
    if (arg == NULL) {
        return;
    }

    codegenPostOrderExpr(arg);
    codegenPushArgsForward(arg->next);
}

static void codegenBuiltinCall(ASTNode *node) {
    const char *name = node->data.funcCall.funcName;
    ASTNode *arg = node->data.funcCall.args;
    const char *frame = codegenFramePrefix();
    char label[64];

    if (strcmp(name, "print") != 0) {
        codegenPushArgsForward(node->data.funcCall.args);
    }

    if (strcmp(name, "print") == 0) {
        codegenPostOrderExpr(arg);
        OUTPUT("POPS %s%s\n", frame, "tmp1s");
        OUTPUT("WRITE %s%s\n", frame, "tmp1s");
    } else if (strcmp(name, "readLine") == 0) {
        OUTPUT("READ %s%s string\n", frame, "tmp1s");
        OUTPUT("PUSHS %s%s\n", frame, "tmp1s");
    } else if (strcmp(name, "readInt") == 0) {
        codegenLabelName("ifj$read$nil", label, sizeof(label));
        char labelDone[64];
        codegenLabelName("ifj$read$done", labelDone, sizeof(labelDone));
        OUTPUT("READ %s%s int\n", frame, "tmp1s");
        OUTPUT("TYPE %s%s %s%s\n", frame, "tmp2s", frame, "tmp1s");
        OUTPUT("JUMPIFEQ %s %s%s string@nil\n", label, frame, "tmp2s");
        OUTPUT("PUSHS %s%s\n", frame, "tmp1s");
        OUTPUT("JUMP %s\n", labelDone);
        OUTPUT("LABEL %s\n", label);
        OUTPUT("PUSHS int@0\n");
        OUTPUT("LABEL %s\n", labelDone);
    } else if (strcmp(name, "readDouble") == 0) {
        codegenLabelName("ifj$read$nil", label, sizeof(label));
        char labelDone[64];
        codegenLabelName("ifj$read$done", labelDone, sizeof(labelDone));
        OUTPUT("READ %s%s float\n", frame, "tmp1s");
        OUTPUT("TYPE %s%s %s%s\n", frame, "tmp2s", frame, "tmp1s");
        OUTPUT("JUMPIFEQ %s %s%s string@nil\n", label, frame, "tmp2s");
        OUTPUT("PUSHS %s%s\n", frame, "tmp1s");
        OUTPUT("JUMP %s\n", labelDone);
        OUTPUT("LABEL %s\n", label);
        OUTPUT("PUSHS float@0x0p+0\n");
        OUTPUT("LABEL %s\n", labelDone);
    } else if (strcmp(name, "toStr") == 0) {
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
            codegenLabelName("ifj$str$true", label, sizeof(label));
            char labelEnd[64];
            codegenLabelName("ifj$str$end", labelEnd, sizeof(labelEnd));
            OUTPUT("POPS %s%s\n", frame, "tmp1s");
            OUTPUT("JUMPIFEQ %s %s%s bool@true\n", label, frame, "tmp1s");
            OUTPUT("MOVE %s%s string@false\n", frame, "tmp1s");
            OUTPUT("JUMP %s\n", labelEnd);
            OUTPUT("LABEL %s\n", label);
            OUTPUT("MOVE %s%s string@true\n", frame, "tmp1s");
            OUTPUT("LABEL %s\n", labelEnd);
            OUTPUT("PUSHS %s%s\n", frame, "tmp1s");
            break;
        }
        default:
            ERROR("toStr: unsupported argument type");
            break;
        }
    } else if (strcmp(name, "toInt") == 0) {
        OUTPUT("FLOAT2INTS\n");
    } else if (strcmp(name, "strLength") == 0) {
        OUTPUT("POPS %s%s\n", frame, "tmp1s");
        OUTPUT("STRLEN %s%s %s%s\n", frame, "tmp2s", frame, "tmp1s");
        OUTPUT("PUSHS %s%s\n", frame, "tmp2s");
    } else if (strcmp(name, "subStr") == 0) {
        // call helper function
        OUTPUT("CALL ifj$subStr\n");
    } else if (strcmp(name, "ord") == 0) {
        char labelOutPop[64];
        char labelOut[64];
        char labelDone[64];
        codegenLabelName("ifj$ord$outpop", labelOutPop, sizeof(labelOutPop));
        codegenLabelName("ifj$ord$out", labelOut, sizeof(labelOut));
        codegenLabelName("ifj$ord$done", labelDone, sizeof(labelDone));
        OUTPUT("POPS %stmp2s\n", frame); // i
        OUTPUT("POPS %stmp1s\n", frame); // s
        OUTPUT("PUSHS %stmp2s\n", frame);
        OUTPUT("PUSHS int@0\n");
        OUTPUT("LTS\n");
        OUTPUT("PUSHS bool@true\n");
        OUTPUT("JUMPIFEQS %s\n", labelOut);
        OUTPUT("PUSHS %stmp2s\n", frame);
        OUTPUT("PUSHS %stmp2s\n", frame);
        OUTPUT("STRLEN %stmp2s %stmp1s\n", frame, frame);
        OUTPUT("PUSHS %stmp2s\n", frame);
        OUTPUT("LTS\n");
        OUTPUT("PUSHS bool@false\n");
        OUTPUT("JUMPIFEQS %s\n", labelOutPop);
        OUTPUT("POPS %stmp2s\n", frame);
        OUTPUT("PUSHS %stmp1s\n", frame);
        OUTPUT("PUSHS %stmp2s\n", frame);
        OUTPUT("STRI2INTS\n");
        OUTPUT("JUMP %s\n", labelDone);
        OUTPUT("LABEL %s\n", labelOutPop);
        OUTPUT("POPS %stmp2s\n", frame);
        OUTPUT("LABEL %s\n", labelOut);
        OUTPUT("PUSHS int@-1\n");
        OUTPUT("LABEL %s\n", labelDone);
    } else if (strcmp(name, "chr") == 0) {
        OUTPUT("INT2CHARS\n");
    } else {
        ERROR("Unknown builtin function");
    }
}

static void codegenFuncCall(ASTNode *node) {
    if (node->type != NODE_FUNC_CALL) {
        ERROR("Called codegenFuncCall with incorrect node type");
    }

    if (codegenIsBuiltin(node->data.funcCall.funcName)) {
        codegenBuiltinCall(node);
        return;
    }

    codegenPushArgsForward(node->data.funcCall.args);
    OUTPUT("CALL %s\n", node->data.funcCall.mangledName);
}

static void codegenReturn(ASTNode *node) {
    if (node->type != NODE_RETURN) {
        ERROR("Called codegenReturn with incorrect node type");
    }

    if (node->data.returnStmt.expr != NULL) {
        codegenPostOrderExpr(node->data.returnStmt.expr);
    }

    codegenEpilogue();
}

static void codegenWalk(ASTNode *node, bool inFuncBody) {
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
            ERROR("Unexpected type as statement");
        }
    }
}

static void codegenSubStrHelper(void) {
    OUTPUT("LABEL ifj$subStr\n");
    OUTPUT("CREATEFRAME\n");
    OUTPUT("PUSHFRAME\n");
    OUTPUT("DEFVAR LF@s\n");
    OUTPUT("DEFVAR LF@i\n");
    OUTPUT("DEFVAR LF@end\n");
    OUTPUT("DEFVAR LF@res\n");
    OUTPUT("DEFVAR LF@tmp\n");
    OUTPUT("POPS LF@end\n");
    OUTPUT("POPS LF@i\n");
    OUTPUT("POPS LF@s\n");
    OUTPUT("LT LF@tmp LF@i int@0\n");
    OUTPUT("JUMPIFEQ ifj$subStr$err LF@tmp bool@true\n");
    OUTPUT("LT LF@tmp LF@end int@0\n");
    OUTPUT("JUMPIFEQ ifj$subStr$err LF@tmp bool@true\n");
    OUTPUT("GT LF@tmp LF@i LF@end\n");
    OUTPUT("JUMPIFEQ ifj$subStr$err LF@tmp bool@true\n");
    OUTPUT("STRLEN LF@res LF@s\n");
    OUTPUT("LT LF@tmp LF@i LF@res\n");
    OUTPUT("JUMPIFEQ ifj$subStr$err LF@tmp bool@false\n");
    OUTPUT("GT LF@tmp LF@end LF@res\n");
    OUTPUT("JUMPIFEQ ifj$subStr$err LF@tmp bool@true\n");
    OUTPUT("MOVE LF@res string@\n");
    OUTPUT("LABEL ifj$subStr$loop\n");
    OUTPUT("JUMPIFEQ ifj$subStr$done LF@i LF@end\n");
    OUTPUT("GETCHAR LF@tmp LF@s LF@i\n");
    OUTPUT("CONCAT LF@res LF@res LF@tmp\n");
    OUTPUT("ADD LF@i LF@i int@1\n");
    OUTPUT("JUMP ifj$subStr$loop\n");
    OUTPUT("LABEL ifj$subStr$err\n");
    OUTPUT("EXIT int@40\n");
    OUTPUT("LABEL ifj$subStr$done\n");
    OUTPUT("PUSHS LF@res\n");
    OUTPUT("POPFRAME\n");
    OUTPUT("RETURN\n");
    OUTPUT("\n");
}

static void codegenParamsReversed(ASTNode *param) {
    if (param == NULL) {
        return;
    }
    codegenParamsReversed(param->next);
    codegenParam(param);
}

static void codegenFunctionDef(ASTNode *node) {
    if (node->type != NODE_FUNC_DEF) {
        ERROR("Called codegenFunctionDef with incorrect node type");
    }

    // add function name
    bool isMain = node->data.funcDef.isMain;
    codegenGeneratingMain = isMain;

    if (isMain) {
        OUTPUT("LABEL main\n");
    }

    OUTPUT("LABEL %s\n", node->data.funcDef.mangledName);

    // main is entered by JUMP and uses global frame
    if (!isMain) {
        OUTPUT("CREATEFRAME\n");
        OUTPUT("PUSHFRAME\n");
    }

    // create parameters
    codegenParamsReversed(node->data.funcDef.params);

    const char *frame = codegenFramePrefix();
    OUTPUT("DEFVAR %s%s\n", frame, "tmp1s");
    OUTPUT("DEFVAR %s%s\n", frame, "tmp2s");

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

    codegenSubStrHelper();

    // walk over functions
    ASTNode *nextFunction = root->data.program.functions;

    while (nextFunction != NULL) {
        codegenFunctionDef(nextFunction);
        nextFunction = nextFunction->next;
    }
}
