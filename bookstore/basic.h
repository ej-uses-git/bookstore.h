// @module Basic definitions and utilities.

#ifndef BASIC_H_
#define BASIC_H_

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

// @macro_function MACRO_STRING
//
// @argument x The argument to quote as a string
// @returns The argument, quoted as a string
//
// @description
// Quote an argument to a macro as a string.
//
// @example
// #define MAC(n) MACRO_STRING(n) "!"
// MAC(hello); // "hello!"
#define MACRO_STRING(x) #x

// @macro global
//
// @description
// Mark a variable as global.
#define global static
// @macro internal
//
// @description
// Mark a function as internal to the file.
#define internal static
// @macro persist
//
// @description
// Mark a variable as persisting between function calls
#define persist static

// @macro_function DEFER_SETUP
//
// @argument T The type returned from the current function
// @argument value The value to return if [DEFER_RETURN] is never called
//
// @description
// Setup the return value so you can call [DEFER_RETURN].
#define DEFER_SETUP(T, value) T defer__result = value
// @macro_function DEFER_LABEL
//
// @argument block The cleanup logic to run before returning.
//
// @description
// Setup the label to go to at the end of the function.
#define DEFER_LABEL(block)                                                     \
    defer__label:                                                              \
    do block while (0);                                                        \
    return defer__result;
// @macro_function DEFER_RETURN
//
// @argument value The value to return from the current function.
//
// @description
// Return from a function, running the cleanup logic from [DEFER_LABEL] first.
#define DEFER_RETURN(value)                                                    \
    do {                                                                       \
        defer__result = value;                                                 \
        goto defer__label;                                                     \
    } while (0)

// @type i8
//
// @description
// An 8 bit signed integer.
typedef int8_t i8;
// @type i16
//
// @description
// A 16 bit signed integer.
typedef int16_t i16;
// @type i32
//
// @description
// A 32 bit signed integer.
typedef int32_t i32;
// @type i64
//
// @description
// A 64 bit signed integer.
typedef int64_t i64;
// @type u8
//
// @description
// An 8 bit unsigned integer.
typedef uint8_t u8;
// @type u16
//
// @description
// A 16 bit unsigned integer.
typedef uint16_t u16;
// @type u32
//
// @description
// A 32 bit unsigned integer.
typedef uint32_t u32;
// @type u64
//
// @description
// A 64 bit unsigned integer.
typedef uint64_t u64;

// @macro I8_FMT
//
// @description
// Format for the [i8] integer type.
//
// @example
// i8 n;
// printf(I8_FMT, n);
#define I8_FMT "%" PRIi8
// @macro I16_FMT
//
// @description
// Format for the [i16] integer type.
//
// @example
// i16 n;
// printf(I16_FMT, n);
#define I16_FMT "%" PRIi16
// @macro I32_FMT
//
// @description
// Format for the [i32] integer type.
//
// @example
// i32 n;
// printf(I32_FMT, n);
#define I32_FMT "%" PRIi32
// @macro I64_FMT
//
// @description
// Format for the [i64] integer type.
//
// @example
// i64 n;
// printf(I64_FMT, n);
#define I64_FMT "%" PRIi64
// @macro U8_FMT
//
// @description
// Format for the [u8] integer type.
//
// @example
// u8 n;
// printf(U8_FMT, n);
#define U8_FMT "%" PRIu8
// @macro U16_FMT
//
// @description
// Format for the [u16] integer type.
//
// @example
// u16 n;
// printf(U16_FMT, n);
#define U16_FMT "%" PRIu16
// @macro U32_FMT
//
// @description
// Format for the [u32] integer type.
//
// @example
// u32 n;
// printf(U32_FMT, n);
#define U32_FMT "%" PRIu32
// @macro U64_FMT
//
// @description
// Format for the [u64] integer type.
//
// @example
// u64 n;
// printf(U64_FMT, n);
#define U64_FMT "%" PRIu64

// @macro_function KiB
// @argument n
// @returns `n` kilobytes
#define KiB(n) ((n) << 10)
// @macro_function MiB
// @argument n
// @returns `n` Megabytes
#define MiB(n) ((n) << 20)
// @macro_function GiB
// @argument n
// @returns `n` Gigabytes
#define GiB(n) ((n) << 30)

// @macro_function MIN
//
// @argument a
// @argument b
// @returns The minimum value between `a` and `b`.
//
// @example
// MIN(0, 1); // 0
#define MIN(a, b) ((a) < (b) ? (a) : (b))
// @macro_function MAX
//
// @argument a
// @argument b
// @returns The maximum value between `a` and `b`.
//
// @example
// MIN(0, 1); // 1
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// @macro_function EQ
//
// @argument a
// @argument b
// @returns Whether `a` and `b` are equal
//
// @description
// Compare using `==`.
#define EQ(a, b) ((a) == (b))
// @macro_function NE
//
// @argument a
// @argument b
// @returns Whether `a` and `b` are not equal
//
// @description
// Compare using `!=`.
#define NE(a, b) ((a) != (b))
// @macro_function LT
//
// @argument a
// @argument b
// @returns Whether `a` is less than `b`
//
// @description
// Compare using `<`.
#define LT(a, b) ((a) < (b))
// @macro_function GT
//
// @argument a
// @argument b
// @returns Whether `a` is greater than `b`
//
// @description
// Compare using `>`.
#define GT(a, b) ((a) > (b))
// @macro_function LTE
//
// @argument a
// @argument b
// @returns Whether `a` is less than or equal to `b`
//
// @description
// Compare using `<=`.
#define LTE(a, b) ((a) <= (b))
// @macro_function GTE
//
// @argument a
// @argument b
// @returns Whether `a` is greater than or equal to `b`
//
// @description
// Compare using `>=`.
#define GTE(a, b) ((a) >= (b))

// @type Order
//
// @description
// An enumeration implying the order between two different values. "Compare" \
// macros and functions should return `Order`, and can then be used to \
// sort data structures.
typedef enum {
    // @enum_value ORDER_LT The first value is less than the second value
    // @enum Order
    ORDER_LT = -1,
    // @enum_value ORDER_EQ The two values are equal
    // @enum Order
    ORDER_EQ = 0,
    // @enum_value ORDER_GT The first value is greater than the second value
    // @enum Order
    ORDER_GT = 1
} Order;
// @macro_function COMPARE_BASIC
//
// @argument a
// @argument b
// @returns An [Order] describing the relationship between `a` and `b`.
//
// @description
// A basic "compare" macro, using `>` and `<` to get an `Order` from two values.
//
// @example
// COMPARE_BASIC(0, 1); // ORDER_LT
// COMPARE_BASIC(0, 0); // ORDER_EQ
// COMPARE_BASIC(1, 0); // ORDER_GT
#define COMPARE_BASIC(a, b)                                                    \
    ((a) < (b) ? ORDER_LT : (a) > (b) ? ORDER_GT : ORDER_EQ)

// @macro_function PRINTF_FORMAT
//
// @argument fmt_index The index of the string `fmt` argument
// @argument check_index The index of the argument to start checking from
//
// @description
// Decorate a function to check for proper `printf` formatting.
//
// @example
// void something_f(i32 n, const char *fmt, i32 y, ...) PRINTF_FORMAT(2, 4);
#if defined(__GNUC__) || defined(__clang__)
//   https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
#ifdef __MINGW_PRINTF_FORMAT
#define PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)                            \
    __attribute__((format(__MINGW_PRINTF_FORMAT, STRING_INDEX, FIRST_TO_CHECK)))
#else
#define PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)                            \
    __attribute__((format(printf, STRING_INDEX, FIRST_TO_CHECK)))
#endif // __MINGW_PRINTF_FORMAT
#else
//   TODO: implement PRINTF_FORMAT for MSVC
#define PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)
#endif

// @macro_function UNREACHABLE
//
// @argument message The message to print if this code path is reached
//
// @description
// Assert that a code path is unreachable.
#define UNREACHABLE(message)                                                   \
    do {                                                                       \
        fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__,        \
                message);                                                      \
        abort();                                                               \
    } while (0)

// @macro_function TODO
//
// @argument message Additional message to print
//
// @description
// Print a TODO message and abort.
#define TODO(message)                                                          \
    do {                                                                       \
        fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, message);     \
        abort();                                                               \
    } while (0)

// @macro_function UNUSED
//
// @argument x
//
// @description
// Mark an identifier as unused so the compiler doesn't complain.
#define UNUSED(x) (void)(x)

// @macro_function IDENTITY
//
// @argument x
// @returns `x`
//
// @description
// Return an expression without modifying it.
#define IDENTITY(x) (x)

// @macro_function ASSERT
//
// @argument cond The condition to assert
// @argument message The message to show if the assertion failed
//
// @description
// Like `assert`, but can be overridden by using a `#define`.
#ifndef ASSERT
#include <assert.h>
#define ASSERT(cond, message) assert(cond &&message)
#endif // ASSERT

// @macro_function MALLOC
//
// @argument size The amount of bytes to allocate
// @returns A pointer to the heap-allocated memory
//
// @description
// Wrapper around standard-library `malloc` which can be overridden using a \
// `#define`.
#ifndef MALLOC
#include <stdlib.h>
#define MALLOC malloc
#endif // MALLOC

// @macro_function REALLOC
//
// @argument ptr The pointer to the existing memory
// @argument size The amount of bytes to allocate
// @returns A pointer to the heap-reallocated memory
//
// @description
// Wrapper around standard-library `realloc` which can be overridden using a \
// `#define`.
#ifndef REALLOC
#include <stdlib.h>
#define REALLOC realloc
#endif // REALLOC

// @macro_function MEMCPY
//
// @argument dst The memory to copy to
// @argument src The memory to copy from
// @argument n The amount of memory to copy
// @returns The original value of `dst`
//
// @description
// Wrapper around standard-library `memcpy` which can be overridden using a \
// `#define`.
#ifndef MEMCPY
#include <string.h>
#define MEMCPY memcpy
#endif // MEMCPY

// @type LogLevel
//
// @description
// Different possible levels to do logging at.
typedef enum LogLevel {
    // @enum_value LOG_TRACE Tracing logs
    // @enum LogLevel
    LOG_TRACE,
    // @enum_value LOG_DEBUG Debug logs
    // @enum LogLevel
    LOG_DEBUG,
    // @enum_value LOG_INFO Informational logs
    // @enum LogLevel
    LOG_INFO,
    // @enum_value LOG_WARN Warning logs
    // @enum LogLevel
    LOG_WARN,
    // @enum_value LOG_ERROR Error logs
    // @enum LogLevel
    LOG_ERROR,
} LogLevel;

// @variable min_log_level
// @description
// The minimum level to do logging at.
global LogLevel min_log_level = LOG_INFO;

// TODO: introduce log handlers

// @function vlog_with_level
//
// @argument level The level to log at
// @argument fmt The `printf`-like format string
// @argument args The `va_list` to format with
//
// @description
// Log a message at a specified log level, with `printf` formatting.
void vlog_with_level(LogLevel level, const char *fmt, va_list args);
// @function log_with_level
//
// @argument level The level to log at
// @argument fmt The `printf`-like format string
// @argument ...rest The arguments to format with
//
// @description
// Log a message at a specified log level, with `printf` formatting.
void log_with_level(LogLevel level, const char *fmt, ...) PRINTF_FORMAT(2, 3);
// @function log_trace
//
// @argument fmt The `printf`-like format string
// @argument ...rest The arguments to format with
//
// @description
// Log a message at `LOG_TRACE` level, with `printf` formatting.
void log_trace(const char *fmt, ...) PRINTF_FORMAT(1, 2);
// @function log_debug
//
// @argument fmt The `printf`-like format string
// @argument ...rest The arguments to format with
//
// @description
// Log a message at `LOG_DEBUG` level, with `printf` formatting.
void log_debug(const char *fmt, ...) PRINTF_FORMAT(1, 2);
// @function log_info
//
// @argument fmt The `printf`-like format string
// @argument ...rest The arguments to format with
//
// @description
// Log a message at `LOG_INFO` level, with `printf` formatting.
void log_info(const char *fmt, ...) PRINTF_FORMAT(1, 2);
// @function log_warn
//
// @argument fmt The `printf`-like format string
// @argument ...rest The arguments to format with
//
// @description
// Log a message at `LOG_WARN` level, with `printf` formatting.
void log_warn(const char *fmt, ...) PRINTF_FORMAT(1, 2);
// @function log_error
//
// @argument fmt The `printf`-like format string
// @argument ...rest The arguments to format with
//
// @description
// Log a message at `LOG_ERROR` level, with `printf` formatting.
void log_error(const char *fmt, ...) PRINTF_FORMAT(1, 2);

#ifdef BOOKSTORE_IMPLEMENTATION

#include <stdlib.h>

void vlog_with_level(LogLevel level, const char *fmt, va_list args) {
    if (level < min_log_level) return;

    switch (level) {
    case LOG_TRACE: fprintf(stderr, "[TRACE] "); break;
    case LOG_DEBUG: fprintf(stderr, "[DEBUG] "); break;
    case LOG_INFO:  fprintf(stderr, "[INFO] "); break;
    case LOG_WARN:  fprintf(stderr, "[WARNING] "); break;
    case LOG_ERROR: fprintf(stderr, "[ERROR] "); break;
    default:        UNREACHABLE("LogLevel");
    }

    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void log_with_level(LogLevel level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vlog_with_level(level, fmt, args);
    va_end(args);
}

void log_trace(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vlog_with_level(LOG_TRACE, fmt, args);
    va_end(args);
}

void log_debug(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vlog_with_level(LOG_DEBUG, fmt, args);
    va_end(args);
}

void log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vlog_with_level(LOG_INFO, fmt, args);
    va_end(args);
}

void log_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vlog_with_level(LOG_WARN, fmt, args);
    va_end(args);
}

void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vlog_with_level(LOG_ERROR, fmt, args);
    va_end(args);
}

#endif // BOOKSTORE_IMPLEMENTATION

#endif // BASIC_H_
