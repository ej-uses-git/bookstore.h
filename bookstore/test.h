/* test.h */
/* Testing utilities */

#ifndef TEST_H_
#define TEST_H_

// Override the definition of `ASSERT` from `basic.h` to use `EXPECT`, which
// means failed asserts will just fail the current test instead of crashing the
// program.
#define ASSERT(cond, message) EXPECT(cond, message)

#include "./basic.h"
#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>

// Maximum `BEFORE_EACH` and `AFTER_EACH` hooks to have active at once.
#ifndef HOOKS_MAX
#define HOOKS_MAX 64
#endif // HOOKS_MAX

struct {
    i32 count;
    jmp_buf items[HOOKS_MAX];
} test__before_hooks = {0};
struct {
    i32 count;
    jmp_buf items[HOOKS_MAX];
} test__after_hooks = {0};
struct {
    jmp_buf it_buf;
    i32 describe_level, fails, oks;
    bool any_failed, should_fail;
} test__main_context = {0};

// Define the entry point for a test program, instead of using `int main(...)`.
// Does the necessary setup and cleanup for the other test-related macros.
#define TEST_MAIN(block)                                                       \
    int main(void) {                                                           \
        do block while (0);                                                    \
        if (test__main_context.any_failed) {                                   \
            log_error("tests failed");                                         \
            return 1;                                                          \
        } else {                                                               \
            log_info("all tests passed");                                      \
            return 0;                                                          \
        }                                                                      \
    }
// Run a block of setup code before each `IT`.
#define BEFORE_EACH(block)                                                     \
    do {                                                                       \
        assert(test__before_hooks.count < HOOKS_MAX &&                         \
               "out of memory for BEFORE_EACH hooks - you can manually "       \
               "increase HOOKS_MAX");                                          \
        if (setjmp(test__before_hooks.items[test__before_hooks.count++])) {    \
            do block while (0);                                                \
            longjmp(test__main_context.it_buf, 1);                             \
        }                                                                      \
    } while (0)
// Run a block of cleanup code after each `IT`.
#define AFTER_EACH(block)                                                      \
    do {                                                                       \
        assert(test__after_hooks.count < HOOKS_MAX &&                          \
               "out of memory for AFTER_EACH hooks - you can manually "        \
               "increase HOOKS_MAX");                                          \
        if (setjmp(test__after_hooks.items[test__after_hooks.count++])) {      \
            do block while (0);                                                \
            longjmp(test__main_context.it_buf, 1);                             \
        }                                                                      \
    } while (0)
// Declare a test suite to run `IT` tests inside of.
#define DESCRIBE(message, block)                                               \
    do {                                                                       \
        const char *describe_label = message;                                  \
        log_info("BEGIN %s", describe_label);                                  \
        i32 fails = test__main_context.fails, oks = test__main_context.oks;    \
        i32 before_hooks = test__before_hooks.count,                           \
            after_hooks = test__after_hooks.count;                             \
        test__main_context.describe_level += 1;                                \
        do block while (0);                                                    \
        test__main_context.describe_level -= 1;                                \
        test__before_hooks.count = before_hooks;                               \
        test__after_hooks.count = after_hooks;                                 \
        LogLevel level;                                                        \
        if (test__main_context.fails > fails) {                                \
            test__main_context.any_failed = true;                              \
            level = LOG_ERROR;                                                 \
        } else {                                                               \
            level = LOG_INFO;                                                  \
        }                                                                      \
        log_with_level(level, "END %s: %d failed, %d ok", describe_label,      \
                       test__main_context.fails - fails,                       \
                       test__main_context.oks - oks);                          \
        if (test__main_context.describe_level == 0) {                          \
            fprintf(stderr, "\n");                                             \
            test__main_context.fails = fails;                                  \
            test__main_context.oks = oks;                                      \
        }                                                                      \
    } while (0)
// Define a test to run.
#define IT(message, block)                                                     \
    do {                                                                       \
        const char *it_label = message;                                        \
        i32 before_hooks = test__before_hooks.count;                           \
        for (i32 i = 0; i < before_hooks; i++) {                               \
            if (setjmp(test__main_context.it_buf) == 0) {                      \
                longjmp(test__before_hooks.items[i], 1);                       \
            }                                                                  \
        }                                                                      \
        if (setjmp(test__main_context.it_buf) == 0) {                          \
            log_info("BEGIN %s %s", describe_label, it_label);                 \
            do block while (0);                                                \
            test__main_context.oks += 1;                                       \
            log_info("END %s %s: ok", describe_label, it_label);               \
        } else {                                                               \
            test__main_context.fails += 1;                                     \
            log_error("END %s %s: fail", describe_label, it_label);            \
        }                                                                      \
        i32 after_hooks = test__after_hooks.count;                             \
        for (i32 i = after_hooks - 1; i >= 0; i--) {                           \
            if (setjmp(test__main_context.it_buf) == 0) {                      \
                longjmp(test__after_hooks.items[i], 1);                        \
            }                                                                  \
        }                                                                      \
    } while (0)
// Define a test to run that is expected to fail.
#define IT_FAIL(message, block)                                                \
    do {                                                                       \
        test__main_context.should_fail = true;                                 \
        const char *it_label = message;                                        \
        i32 before_hooks = test__before_hooks.count;                           \
        for (i32 i = 0; i < before_hooks; i++) {                               \
            if (setjmp(test__main_context.it_buf) == 0) {                      \
                longjmp(test__before_hooks.items[i], 1);                       \
            }                                                                  \
        }                                                                      \
        if (setjmp(test__main_context.it_buf) == 0) {                          \
            log_info("BEGIN %s %s", describe_label, it_label);                 \
            do block while (0);                                                \
            test__main_context.fails += 1;                                     \
            log_error("END %s %s: ok (unexpected)", describe_label, it_label); \
        } else {                                                               \
            test__main_context.oks += 1;                                       \
            log_info("END %s %s: fail (expected)", describe_label, it_label);  \
        }                                                                      \
        i32 after_hooks = test__after_hooks.count;                             \
        for (i32 i = after_hooks - 1; i >= 0; i--) {                           \
            if (setjmp(test__main_context.it_buf) == 0) {                      \
                longjmp(test__after_hooks.items[i], 1);                        \
            }                                                                  \
        }                                                                      \
        test__main_context.should_fail = false;                                \
    } while (0)
// Expect a certain condition, with some message in case it fails.
#define EXPECT(cond, message) EXPECTF(cond, "%s", message)
// Expect a certain condition, with some message in case it fails, using `fmt`
// and `printf` formatting.
#define EXPECTF(cond, fmt, ...)                                                \
    do {                                                                       \
        if (!(cond)) FAILF(fmt, __VA_ARGS__);                                  \
    } while (0)
// Expect that a certain boolean condition is `true`, with a default message in
// case it is `false`.
#define EXPECT_TRUE(cond) EXPECT(cond, "false != true")
// Expect that a certain boolean condition is `false`, with a default message in
// case it is `true`.
#define EXPECT_FALSE(cond) EXPECT(!(cond), "true != false")
// Expect that a certain pointer is not `NULL`, with a default message in case
// it is.
#define EXPECT_NON_NULL(ptr) EXPECT(ptr, "unexpected NULL")
// Expect that a certain pointer is `NULL`, with a default message in case it
// isn't.
#define EXPECT_NULL(ptr) EXPECT((ptr) == NULL, "unexpected non-NULL pointer")
// Expect that two elements are equal, with some `printf` format (e.g. `"%d"`)
// to use for a default message in case they aren't equal.
#define EXPECT_EQ(a, b, fmt) EXPECTF((a) == (b), fmt " != " fmt, a, b)
// Expect that two elements are not equal, with some `printf` format (e.g.
// `"%d"`) to use for a default message in case they are equal.
#define EXPECT_NE(a, b, fmt) EXPECTF((a) != (b), fmt " == " fmt, a, b)
// Expect that `a` is greater than `b`, with some `printf` format (e.g. `"%d"`)
// to use for a default message in case it isn't.
#define EXPECT_GT(a, b, fmt) EXPECTF((a) > (b), fmt " <= " fmt, a, b)
// Expect that `a` is greater than or equal to `b`, with some `printf` format
// (e.g. `"%d"`) to use for a default message in case it isn't.
#define EXPECT_GTE(a, b, fmt) EXPECTF((a) >= (b), fmt " < " fmt, a, b)
// Expect that `a` is less than `b`, with some `printf` format (e.g. `"%d"`) to
// use for a default message in case it isn't.
#define EXPECT_LT(a, b, fmt) EXPECTF((a) < (b), fmt " >= " fmt, a, b)
// Expect that `a` is less than or equal to `b`, with some `printf` format (e.g.
// `"%d"`) to use for a default message in case it isn't.
#define EXPECT_LTE(a, b, fmt) EXPECTF((a) <= (b), fmt " > " fmt, a, b)
// Expect that `a` and `b` are equal, using `eq` to compare between the two,
// with some `printf` format and with `map` called on both `a` and `b` before
// passing to be formatted.
//
// Example:
//
// ```
// #define CSTR_EQ(a, b) (strcmp(a, b) == 0)
// EXPECT_EQ_FN("a", "b", CSTR_EQ, "\"%s\"", IDENTITY) // "a" != "b"
// ```
#define EXPECT_EQ_FN(a, b, eq, fmt, map)                                       \
    EXPECTF(eq(a, b), fmt " != " fmt, map(a), map(b))
// Expect that `a` and `b` are not equal, negating `eq` to compare between the
// two, with some `printf` format and with `map` called on both `a` and `b`
// before passing to be formatted.
//
// Example:
//
// ```
// #define CSTR_EQ(a, b) (strcmp(a, b) == 0)
// EXPECT_NE_FN("", "", CSTR_EQ, "\"%s\"", IDENTITY) // "" == ""
// ```
#define EXPECT_NE_FN(a, b, eq, fmt, map)                                       \
    EXPECTF(!eq(a, b), fmt " == " fmt, map(a), map(b))
// Expect that `a` and `b` are equal, with some `printf` format and with `map`
// called on both `a` and `b` before passing to be formatted.
//
// Example:
//
// ```
// #define INC(a) ((a) + 1)
// EXPECT_EQ_MAP(10, 11, "%d", INC) // 11 != 12
// ```
#define EXPECT_EQ_MAP(a, b, fmt, map)                                          \
    EXPECTF((a) == (b), fmt " != " fmt, map(a), map(b))
// Expect that `a` and `b` are not equal, with some `printf` format and with
// `map` called on both `a` and `b` before passing to be formatted.
//
// Example:
//
// ```
// #define INC(a) ((a) + 1)
// EXPECT_EQ_MAP(10, 11, "%d", INC) // 11 != 12
// ```
#define EXPECT_NE_MAP(a, b, fmt, map)                                          \
    EXPECTF((a) != (b), fmt " == " fmt, map(a), map(b))
// Expect that two `StringView` instances are equal, with some default message
// in case they aren't.
#define EXPECT_SV_EQ(a, b) EXPECT_EQ_FN(a, b, sv_eq, "\"" SV_FMT "\"", SV_ARG)
// Expect a `StringView` to be equal to some C-string (NUL-terminated list of
// characters).
#define EXPECT_SV_EQ_CSTR(a, b) EXPECT_SV_EQ(a, sv_from_cstr(b))
// Fail a test with some message.
#define FAIL(message) FAILF("%s", message)
// Fail a test with some message, using `fmt` and `printf` formatting.
#define FAILF(fmt, ...)                                                        \
    do {                                                                       \
        log_with_level(test__main_context.should_fail ? LOG_INFO : LOG_ERROR,  \
                       "%s:%d: " fmt, __FILE__, __LINE__, __VA_ARGS__);        \
        longjmp(test__main_context.it_buf, 1);                                 \
    } while (0)

#endif // TEST_H_
