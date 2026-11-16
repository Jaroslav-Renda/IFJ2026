/**
 * @file    error.h
 * @brief   Implementation of global error handling
 * @author  Jaroslav Renda (xrendaj00)
 * @details project_IFJ2026
 */

#ifndef ERROR_H
#define ERROR_H

/**
 * @brief Print out error from lexical analysis and set return number to 1.
 *
 * @param lineNumber Line number the error occured on.
 * @param str Specifics for the error to print out.
 */
void errorLexer(int lineNumber, const char *str);

/**
 * @brief Print out error from syntactic analysis and set return number to 2.
 *
 * Invalid syntax, missing prolog, forbidden construct or a broken structure
 * of indented blocks.
 *
 * @param lineNumber Line number the error occurred on.
 * @param str Specifics for the error to print out.
 */
void errorParser(int lineNumber, const char *str);

/**
 * @brief Print out semantic error and set return number to 3.
 *
 * Use of an undefined function, variable or type; missing function
 * annotated with @main.
 *
 * @param lineNumber Line number the error occurred on.
 * @param str Specifics for the error to print out.
 */
void errorSemUndefined(int lineNumber, const char *str);

/**
 * @brief Print out semantic error and set return number to 4.
 *
 * Forbidden redefinition of a function, variable or type.
 *
 * @param lineNumber Line number the error occurred on.
 * @param str Specifics for the error to print out.
 */
void errorSemRedefinition(int lineNumber, const char *str);

/**
 * @brief Print out semantic error and set return number to 5.
 *
 * Wrong number or types of arguments in a function call; forbidden parameters
 * of the function annotated with @main.
 *
 * @param lineNumber Line number the error occurred on.
 * @param str Specifics for the error to print out.
 */
void errorSemArguments(int lineNumber, const char *str);

/**
 * @brief Print out semantic type error and set return number to 6.
 *
 * Type compatibility or type inference error in an expression, assignment,
 * initialization, return value or control construct.
 *
 * @param lineNumber Line number the error occurred on.
 * @param str Specifics for the error to print out.
 */
void errorSemType(int lineNumber, const char *str);

/**
 * @brief Print out semantic error and set return number to 7.
 *
 * Attempt to write to an immutable variable or a formal function parameter.
 *
 * @param lineNumber Line number the error occurred on.
 * @param str Specifics for the error to print out.
 */
void errorSemImmutable(int lineNumber, const char *str);

/**
 * @brief Print out other semantic error and set return number to 10.
 *
 * E.g. forbidden return, non-exhaustive match or a zero range step.
 *
 * @param lineNumber Line number the error occurred on.
 * @param str Specifics for the error to print out.
 */
void errorSemOther(int lineNumber, const char *str);

/**
 * @brief Print out internal compiler error and set return number to 99.
 *
 * The error is not influenced by the input program, e.g. memory allocation
 * failure.
 *
 * @param str Specifics for the error to print out.
 */
void errorInternal(const char *str);

#endif // !ERROR_H
