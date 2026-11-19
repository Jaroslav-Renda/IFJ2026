/**
 * @file    prectable.h
 * @brief   Representation of precedence table
 * @author  Jaroslav Renda (xrendaj00)
 * @details project_IFJ2026
 */

#ifndef PRECTABLE_H
#define PRECTABLE_H

#include <stdint.h>

#define TABLE_SIZE 17

/**
 * @brief Precedence table relations between items
 */
typedef enum : uint8_t {
    S, /// Shift
    R, /// Reduce
    E, /// Error (Not part of language L)
    Q, /// Operators are equal (shift without stop)
    A, /// If the expression is finished (successfully)
} precRelations;

/**
 * @brief Precedence operators for expressions
 */
typedef enum : uint8_t {
    P_ID,   // id
    P_NEG,  // -E
    P_DIV,  // /
    P_TIME, // *
    P_MIN,  // -
    P_PLS,  // +
    P_LT,   // <
    P_LET,  // <=
    P_GET,  // >=
    P_GT,   // >
    P_EQ,   // ==
    P_NEQ,  // !=
    P_LPAR, // (
    P_RPAR, // )
    P_COM,  // ,
    P_LIT,  // int, double, string
    P_END   // $
} precOperators;

/**
 * @brief The precedance table
 *
 * An 2D array of precedence ralations between operators. The operators are in
 *
 * For each operator a row means that the operator is on the stack, and collumn
 * means it is an incoming token.
 */
const precRelations precTable[TABLE_SIZE][TABLE_SIZE] = {
    // clang-format off
    // NOLINTBEGIN

    //P_ID, P_NEG, P_DIV, P_TIME, P_MIN, P_PLS, P_LT, P_LET, P_GET, P_GT, P_EQ, P_NEQ, P_LPAR, P_RPAR, P_COM, P_LIT, P_END
    {E, E, R, R, R, R, R, R, R, R, R, R, Q, R, R, E, R}, // P_ID
    {S, S, R, R, R, R, R, R, R, R, R, R, S, R, R, S, R}, // P_NEG
    {S, S, R, R, R, R, R, R, R, R, R, R, S, R, R, S, R}, // P_DIV
    {S, S, R, R, R, R, R, R, R, R, R, R, S, R, R, S, R}, // P_TIME
    {S, S, S, S, R, R, R, R, R, R, R, R, S, R, R, S, R}, // P_MIN
    {S, S, S, S, R, R, R, R, R, R, R, R, S, R, R, S, R}, // P_PLS
    {S, S, S, S, S, S, E, E, E, E, E, E, S, R, R, S, R}, // P_LT
    {S, S, S, S, S, S, E, E, E, E, E, E, S, R, R, S, R}, // P_LET
    {S, S, S, S, S, S, E, E, E, E, E, E, S, R, R, S, R}, // P_GET
    {S, S, S, S, S, S, E, E, E, E, E, E, S, R, R, S, R}, // P_GT
    {S, S, S, S, S, S, E, E, E, E, E, E, S, R, R, S, R}, // P_EQ
    {S, S, S, S, S, S, E, E, E, E, E, E, S, R, R, S, R}, // P_NEQ
    {S, S, S, S, S, S, S, S, S, S, S, S, S, Q, S, S, E}, // P_LPAR
    {E, E, R, R, R, R, R, R, R, R, R, R, E, R, R, E, R}, // P_RPAR
    {S, S, S, S, S, S, S, S, S, S, S, S, S, R, R, S, E}, // P_COM
    {E, E, R, R, R, R, R, R, R, R, R, R, E, R, R, E, R}, // P_LIT
    {S, S, S, S, S, S, S, S, S, S, S, S, S, E, E, S, A}, // P_END

    // NOLINTEND
    // clang-format on

};

#endif // !PRECTABLE_H
