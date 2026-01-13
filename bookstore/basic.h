/* basic.h */
/* Basic definitions and utilities */

#ifndef BASIC_H_
#define BASIC_H_

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

// `static`-related definitions

// Mark a variable as global.
#define global static
// Mark a function as internal to the file.
#define internal static
// Mark a variable as persisting between function calls
#define persist static

// Defer utilities - simple defer pattern for functions

// Setup the return value so you can call `DEFER_RETURN`.
// `T` is the type returned from the current function;
// `value` is the value to return if `DEFER_RETURN` is never called.
#define DEFER_SETUP(T, value) T defer__result = value
// Setup the label to go to at the end of the function.
// The `block` is the cleanup logic to run before returning.
#define DEFER_LABEL(block)                                                     \
    defer__label:                                                              \
    do block while (0);                                                        \
    return defer__result;
// Return from a function, running the cleanup logic from `DEFER_LABEL` first.
#define DEFER_RETURN(value)                                                    \
    do {                                                                       \
        defer__result = value;                                                 \
        goto defer__label;                                                     \
    } while (0)

// Integer type definitions

// An 8 bit signed integer.
typedef int8_t i8;
// A 16 bit signed integer.
typedef int16_t i16;
// A 32 bit signed integer.
typedef int32_t i32;
// A 64 bit signed integer.
typedef int64_t i64;
// An 8 bit unsigned integer.
typedef uint8_t u8;
// A 16 bit unsigned integer.
typedef uint16_t u16;
// A 32 bit unsigned integer.
typedef uint32_t u32;
// A 64 bit unsigned integer.
typedef uint64_t u64;

// Integer type formats

// Format for the `i8` integer type.
#define I8_FMT "%" PRIi8
// Format for the `i16` integer type.
#define I16_FMT "%" PRIi16
// Format for the `i32` integer type.
#define I32_FMT "%" PRIi32
// Format for the `i64` integer type.
#define I64_FMT "%" PRIi64
// Format for the `u8` integer type.
#define U8_FMT "%" PRIu8
// Format for the `u16` integer type.
#define U16_FMT "%" PRIu16
// Format for the `u32` integer type.
#define U32_FMT "%" PRIu32
// Format for the `u64` integer type.
#define U64_FMT "%" PRIu64

// Sizing in bytes

// Kilobytes
#define KiB(n) ((n) << 10)
// Megabytes
#define MiB(n) ((n) << 20)
// Gigabytes
#define GiB(n) ((n) << 30)

// Useful utilities

// Get the minimum of two expressions
#define MIN(a, b) ((a) < (b) ? (a) : (b))
// Get the maximum of two expressions
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Comparison operators as macros, so they can be passed to other macros

// EQ - compare using `==`
#define EQ(a, b) ((a) == (b))
// NE - compare using `!=`
#define NE(a, b) ((a) != (b))
// LT - compare using `<`
#define LT(a, b) ((a) < (b))
// GT - compare using `>`
#define GT(a, b) ((a) > (b))
// LTE - compare using `<=`
#define LTE(a, b) ((a) <= (b))
// GTE - compare using `>=`
#define GTE(a, b) ((a) >= (b))

// Ordering logic

// An enumeration implying the order between two different values. "Compare"
// macros and functions should return this enumeration, and can then be used to
// sort data structures.
typedef enum {
    // The first value is less than the second value.
    ORDER_LT = -1,
    // The two values are equal.
    ORDER_EQ = 0,
    // The first value is greater than the second value.
    ORDER_GT = 1
} Order;
// A basic "compare" macro, using `>` and `<` to get an `Order` from two values.
#define COMPARE_BASIC(a, b)                                                    \
    ((a) < (b) ? ORDER_LT : (a) > (b) ? ORDER_GT : ORDER_EQ)

// PRINTF_FORMAT - decorate a function to check for proper `printf` formatting:
// the first argument is the index of the string `fmt` argument,
// and the second argument is the index of the argument to start checking from.
// For an example you can look at the definition of `log_with_level`.
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

// UNREACHABLE - assert that a code path is unreachable
#define UNREACHABLE(message)                                                   \
    do {                                                                       \
        fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__,        \
                message);                                                      \
        abort();                                                               \
    } while (0)

// TODO - print a TODO message and abort
#define TODO(message)                                                          \
    do {                                                                       \
        fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, message);     \
        abort();                                                               \
    } while (0)

// UNUSED - mark a parameter as unused so the compiler doesn't complain
#define UNUSED(x) (void)(x)

// IDENTITY - return an expression without modifying it
#define IDENTITY(x) (x)

// ASSERT - assertion, which can be overridden by using a #define
#ifndef ASSERT
#include <assert.h>
#define ASSERT(cond, message) assert(cond &&message)
#endif // ASSERT

// Memory functions which can be overridden by using a #define

// MALLOC - standard library `malloc`
#ifndef MALLOC
#include <stdlib.h>
#define MALLOC malloc
#endif // MALLOC

// REALLOC - standard library `malloc`
#ifndef REALLOC
#include <stdlib.h>
#define REALLOC realloc
#endif // REALLOC

#ifndef MEMCPY
#include <string.h>
#define MEMCPY memcpy
#endif // MEMCPY

// Logging utilities

// Different possible levels to do logging at.
typedef enum LogLevel {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
} LogLevel;

// The minimum level to do logging to standard error
global LogLevel min_log_level = LOG_INFO;

// Log a message at some `LogLevel`, with `printf` formatting using a `va_list`.
void vlog_with_level(LogLevel level, const char *fmt, va_list args);
// Log a message at some `LogLevel`, with `printf` formatting.
void log_with_level(LogLevel level, const char *fmt, ...) PRINTF_FORMAT(2, 3);
// Log a message at `LOG_TRACE` level, with `printf` formatting.
void log_trace(const char *fmt, ...) PRINTF_FORMAT(1, 2);
// Log a message at `LOG_DEBUG` level, with `printf` formatting.
void log_debug(const char *fmt, ...) PRINTF_FORMAT(1, 2);
// Log a message at `LOG_INFO` level, with `printf` formatting.
void log_info(const char *fmt, ...) PRINTF_FORMAT(1, 2);
// Log a message at `LOG_WARN` level, with `printf` formatting.
void log_warn(const char *fmt, ...) PRINTF_FORMAT(1, 2);
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
