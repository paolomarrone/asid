/* common.h only needs INFINITY; A-SID supplies its own math functions. */
#ifndef ASID_WASM_MATH_H
#define ASID_WASM_MATH_H
#define INFINITY (__builtin_inff())
#endif
