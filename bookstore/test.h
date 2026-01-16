/* test.h */
/* Testing utilities */

#ifndef TEST_H_
#define TEST_H_

#include <stdbool.h>

// Forward-declare wrapper for `EXPECT` so we can override `ASSERT` before
// including other files
void test__expect(bool cond, const char *message);

// Override the definition of `ASSERT` from `basic.h` to use `EXPECT`, which
// means failed asserts will just fail the current test instead of crashing the
// program.
#define ASSERT(cond, message) test__expect(cond, message)

#include "./array.h"
#include "./basic.h"
#include "./string.h"
#include <assert.h>
#include <setjmp.h>

// Maximum `BEFORE_EACH` and `AFTER_EACH` hooks to have active at once.
#ifndef TEST_MAX_HOOKS
#define TEST_MAX_HOOKS 64
#endif // TEST_MAX_HOOKS

#ifndef TEST_MAX_DEPTH
#define TEST_MAX_DEPTH 12
#endif // TEST_MAX_DEPTH

#ifndef TEST_MAX_LABEL_LENGTH
#define TEST_MAX_LABEL_LENGTH 1024
#endif // TEST_MAX_LABEL_LENGTH

#define TEST__MAX_RENDERED_LABEL (TEST_MAX_LABEL_LENGTH * (TEST_MAX_DEPTH + 2))

// Define the entry point for a test program, instead of using `int main(...)`.
// Does the necessary setup and cleanup for the other test-related macros.
#define TEST_MAIN(block)                                                       \
    int main(void) {                                                           \
        Arena *test__arena =                                                   \
            arena_new((sizeof(Test__Label) * TEST_MAX_DEPTH) +                 \
                      TEST__MAX_RENDERED_LABEL);                               \
        test__context.describe_labels =                                        \
            test__labels_new(test__arena, TEST_MAX_DEPTH);                     \
        do block while (0);                                                    \
        if (test__context.fails > 0) {                                         \
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
        assert(test__context.before_hooks.count < TEST_MAX_HOOKS &&            \
               "out of memory for BEFORE_EACH hooks - you can manually "       \
               "increase TEST_MAX_HOOKS");                                     \
        if (setjmp(test__context.before_hooks                                  \
                       .items[test__context.before_hooks.count++])) {          \
            do block while (0);                                                \
            longjmp(test__context.it_buf, 1);                                  \
        }                                                                      \
    } while (0)
// Run a block of cleanup code after each `IT`.
#define AFTER_EACH(block)                                                      \
    do {                                                                       \
        assert(test__context.after_hooks.count < TEST_MAX_HOOKS &&             \
               "out of memory for AFTER_EACH hooks - you can manually "        \
               "increase TEST_MAX_HOOKS");                                     \
        if (setjmp(test__context.after_hooks                                   \
                       .items[test__context.after_hooks.count++])) {           \
            do block while (0);                                                \
            longjmp(test__context.it_buf, 1);                                  \
        }                                                                      \
    } while (0)
// Declare a test suite to run `IT` tests inside of.
#define DESCRIBE(message, block)                                               \
    do {                                                                       \
        assert(strlen((message)) < TEST_MAX_LABEL_LENGTH &&                    \
               "DESCRIBE message too long - you can manually increase "        \
               "TEST_MAX_LABEL_LENGTH");                                       \
        test__context.start_fails = test__context.fails;                       \
        test__context.start_oks = test__context.oks;                           \
        test__context.start_before_hooks = test__context.before_hooks.count;   \
        test__context.start_after_hooks = test__context.after_hooks.count;     \
        test__labels_push(&test__context.describe_labels, message);            \
        {                                                                      \
            Lifetime test__lt = lifetime_begin(test__arena);                   \
            StringBuilder test__sb =                                           \
                sb_new(test__lt.arena, TEST__MAX_RENDERED_LABEL);              \
            test__labels_render(&test__sb, test__context.describe_labels);     \
            log_debug("DESCRIBE: " SB_FMT, SB_ARG(test__sb));                  \
            lifetime_end(test__lt);                                            \
        }                                                                      \
        do block while (0);                                                    \
        test__context.before_hooks.count = test__context.start_before_hooks;   \
        test__context.after_hooks.count = test__context.start_after_hooks;     \
        LogLevel level;                                                        \
        if (test__context.fails > test__context.start_fails) {                 \
            level = LOG_ERROR;                                                 \
        } else {                                                               \
            level = LOG_INFO;                                                  \
        }                                                                      \
        {                                                                      \
            Lifetime test__lt = lifetime_begin(test__arena);                   \
            StringBuilder test__sb =                                           \
                sb_new(test__lt.arena, TEST__MAX_RENDERED_LABEL);              \
            test__labels_render(&test__sb, test__context.describe_labels);     \
            log_with_level(level, "%s:%d: " SB_FMT ": %d failed, %d ok",       \
                           __FILE__, __LINE__, SB_ARG(test__sb),               \
                           test__context.fails - test__context.start_fails,    \
                           test__context.oks - test__context.start_oks);       \
            lifetime_end(test__lt);                                            \
        }                                                                      \
        test__labels_pop(&test__context.describe_labels);                      \
        if (test__context.describe_labels.count == 0) {                        \
            fprintf(stderr, "\n");                                             \
            test__context.fails = test__context.start_fails;                   \
            test__context.oks = test__context.start_oks;                       \
        }                                                                      \
    } while (0)
// Define a test to run.
#define IT(message, block)                                                     \
    do {                                                                       \
        for (i32 i = 0; i < test__context.before_hooks.count; i++) {           \
            if (setjmp(test__context.it_buf) == 0) {                           \
                longjmp(test__context.before_hooks.items[i], 1);               \
            }                                                                  \
        }                                                                      \
        if (setjmp(test__context.it_buf) == 0) {                               \
            {                                                                  \
                Lifetime test__lt = lifetime_begin(test__arena);               \
                StringBuilder test__sb =                                       \
                    sb_new(test__lt.arena, TEST__MAX_RENDERED_LABEL);          \
                test__labels_render_with_it(                                   \
                    &test__sb, test__context.describe_labels, (message));      \
                log_debug("IT: " SB_FMT, SB_ARG(test__sb));                    \
                lifetime_end(test__lt);                                        \
            }                                                                  \
            do block while (0);                                                \
            test__context.oks += 1;                                            \
            {                                                                  \
                Lifetime test__lt = lifetime_begin(test__arena);               \
                StringBuilder test__sb =                                       \
                    sb_new(test__lt.arena, TEST__MAX_RENDERED_LABEL);          \
                test__labels_render_with_it(                                   \
                    &test__sb, test__context.describe_labels, (message));      \
                log_info("%s:%d: " SB_FMT, __FILE__, __LINE__,                 \
                         SB_ARG(test__sb));                                    \
                lifetime_end(test__lt);                                        \
            }                                                                  \
        } else {                                                               \
            test__context.fails += 1;                                          \
            {                                                                  \
                Lifetime test__lt = lifetime_begin(test__arena);               \
                StringBuilder test__sb =                                       \
                    sb_new(test__lt.arena, TEST__MAX_RENDERED_LABEL);          \
                test__labels_render_with_it(                                   \
                    &test__sb, test__context.describe_labels, (message));      \
                log_error("%s:%d: " SB_FMT, __FILE__, __LINE__,                \
                          SB_ARG(test__sb));                                   \
                lifetime_end(test__lt);                                        \
            }                                                                  \
        }                                                                      \
        for (i32 i = test__context.after_hooks.count - 1; i >= 0; i--) {       \
            if (setjmp(test__context.it_buf) == 0) {                           \
                longjmp(test__context.after_hooks.items[i], 1);                \
            }                                                                  \
        }                                                                      \
    } while (0)
// Define a test to run that is expected to fail.
#define IT_FAIL(message, block)                                                \
    do {                                                                       \
        test__context.should_fail = true;                                      \
        for (i32 i = 0; i < test__context.before_hooks.count; i++) {           \
            if (setjmp(test__context.it_buf) == 0) {                           \
                longjmp(test__context.before_hooks.items[i], 1);               \
            }                                                                  \
        }                                                                      \
        if (setjmp(test__context.it_buf) == 0) {                               \
            {                                                                  \
                Lifetime test__lt = lifetime_begin(test__arena);               \
                StringBuilder test__sb =                                       \
                    sb_new(test__lt.arena, TEST__MAX_RENDERED_LABEL);          \
                test__labels_render_with_it(                                   \
                    &test__sb, test__context.describe_labels, (message));      \
                log_debug("IT_FAIL: " SB_FMT, SB_ARG(test__sb));               \
                lifetime_end(test__lt);                                        \
            }                                                                  \
            do block while (0);                                                \
            test__context.fails += 1;                                          \
            {                                                                  \
                Lifetime test__lt = lifetime_begin(test__arena);               \
                StringBuilder test__sb =                                       \
                    sb_new(test__lt.arena, TEST__MAX_RENDERED_LABEL);          \
                test__labels_render_with_it(                                   \
                    &test__sb, test__context.describe_labels, (message));      \
                log_info("%s:%d: " SB_FMT " (unexpected success)", __FILE__,   \
                         __LINE__, SB_ARG(test__sb));                          \
                lifetime_end(test__lt);                                        \
            }                                                                  \
        } else {                                                               \
            {                                                                  \
                Lifetime test__lt = lifetime_begin(test__arena);               \
                StringBuilder test__sb =                                       \
                    sb_new(test__lt.arena, TEST__MAX_RENDERED_LABEL);          \
                test__labels_render_with_it(                                   \
                    &test__sb, test__context.describe_labels, (message));      \
                log_info("%s:%d: " SB_FMT " (expected failure)", __FILE__,     \
                         __LINE__, SB_ARG(test__sb));                          \
                lifetime_end(test__lt);                                        \
            }                                                                  \
            test__context.oks += 1;                                            \
        }                                                                      \
        for (i32 i = test__context.after_hooks.count - 1; i >= 0; i--) {       \
            if (setjmp(test__context.it_buf) == 0) {                           \
                longjmp(test__context.after_hooks.items[i], 1);                \
            }                                                                  \
        }                                                                      \
        test__context.should_fail = false;                                     \
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
        log_with_level(test__context.should_fail ? LOG_INFO : LOG_ERROR,       \
                       "%s:%d: " fmt, __FILE__, __LINE__, __VA_ARGS__);        \
        longjmp(test__context.it_buf, 1);                                      \
    } while (0)

typedef struct {
    jmp_buf items[TEST_MAX_HOOKS];
    i32 count;
} Test__Hooks;

typedef const char *Test__Label;
ARRAY_TYPEDEF(Test__Label, Test__Labels);
ARRAY_DECLARE_PREFIX(Test__Label, Test__Labels, test__labels);

typedef struct {
    jmp_buf it_buf;
    Test__Hooks before_hooks;
    Test__Hooks after_hooks;
    Test__Labels describe_labels;
    Test__Label it_label;
    i32 start_before_hooks, start_after_hooks;
    i32 fails, start_fails, oks, start_oks;
    bool should_fail;
} Test__InternalContext;

void test__labels_render(StringBuilder *test__sb, Test__Labels labels);
void test__labels_render_with_it(StringBuilder *test__sb, Test__Labels labels,
                                 Test__Label it_label);

global Test__InternalContext test__context = {0};

#ifdef BOOKSTORE_IMPLEMENTATION

ARRAY_DEFINE_PREFIX(Test__Label, Test__Labels, test__labels)

void test__expect(bool cond, const char *message) {
    EXPECT(cond, message);
}

void test__labels_render(StringBuilder *sb, Test__Labels labels) {
    i32 label_count = labels.count;
    for (i32 i = 0; i < label_count; i++) {
        if (i) sb_push(sb, ' ');
        sb_append_cstr(sb, test__labels_get(labels, i));
    }
}

void test__labels_render_with_it(StringBuilder *sb, Test__Labels labels,
                                 Test__Label it_label) {
    test__labels_render(sb, labels);
    if (labels.count) sb_push(sb, ' ');
    sb_append_cstr(sb, it_label);
}

#endif // BOOKSTORE_IMPLEMENTATION

#endif // TEST_H_
