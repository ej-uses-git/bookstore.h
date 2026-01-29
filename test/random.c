#include "../bookstore/test.h"

#include "../bookstore/random.h"
#include <ctype.h>
#include <time.h>

#define DESCRIBE_NEXT_UINT(T, max, fmt, ...)                                   \
    DESCRIBE("random_next_" MACRO_STRING(T), {                                 \
        PROP(                                                                  \
            "should generate an unsigned number", rng,                         \
            {                                                                  \
                T n = random_next_##T(&rng);                                   \
                EXPECT_LTE(n, max, fmt);                                       \
            },                                                                 \
            __VA_ARGS__);                                                      \
    })

#define DESCRIBE_NEXT_UINT_BOUNDED(T, fmt, ...)                                \
    DESCRIBE("random_next_" MACRO_STRING(T) "_bounded", {                      \
        PROP(                                                                  \
            "should generate a number within the bound", rng,                  \
            {                                                                  \
                T bound = random_next_##T(&rng);                               \
                bound = MAX(bound, 1);                                         \
                T n = random_next_##T##_bounded(&rng, bound);                  \
                EXPECT_LT(n, bound, fmt);                                      \
            },                                                                 \
            __VA_ARGS__);                                                      \
    })

#define DESCRIBE_NEXT_INT(T, max, min, fmt, ...)                               \
    DESCRIBE("random_next_" MACRO_STRING(T), {                                 \
        PROP(                                                                  \
            "should generate an integer", rng,                                 \
            {                                                                  \
                T n = random_next_##T(&rng);                                   \
                EXPECT_LTE(n, max, fmt);                                       \
                EXPECT_GTE(n, min, fmt);                                       \
            },                                                                 \
            __VA_ARGS__);                                                      \
    })

#define DESCRIBE_NEXT_INT_RANGED(T, fmt, ...)                                  \
    DESCRIBE("random_next_" MACRO_STRING(T) "_ranged", {                       \
        PROP(                                                                  \
            "should generate a number within the range", rng,                  \
            {                                                                  \
                T a = random_next_##T(&rng);                                   \
                T b = random_next_##T(&rng);                                   \
                T start = MIN(a, b);                                           \
                T end = MAX(a, b);                                             \
                T n = random_next_##T##_ranged(&rng, start, end);              \
                EXPECT_GTE(n, start, fmt);                                     \
                EXPECT_LTE(n, end, fmt);                                       \
            },                                                                 \
            __VA_ARGS__);                                                      \
    })

#define DESCRIBE_NEXT_CHAR_KIND(kind, article, desc, ...)                      \
    DESCRIBE("random_next_" MACRO_STRING(kind), {                              \
        PROP(                                                                  \
            "should generate " article " " desc " character", rng,             \
            {                                                                  \
                char c = random_next_##kind(&rng);                             \
                EXPECTF(is##kind(c), "'%c' is not " desc, c);                  \
            },                                                                 \
            __VA_ARGS__);                                                      \
    })

TEST_MAIN({
    DESCRIBE_NEXT_UINT(u32, UINT32_MAX, U32_FMT);
    DESCRIBE_NEXT_UINT_BOUNDED(u32, U32_FMT);

    DESCRIBE("random_next_bool", {
        PROP("should generate a random boolean", rng, {
            bool b = random_next_bool(&rng);
            EXPECTF(b == false || b == true, "%d is not a boolean", b);
        });
    });

    DESCRIBE_NEXT_UINT(u8, UINT8_MAX, U8_FMT);
    DESCRIBE_NEXT_UINT_BOUNDED(u8, U8_FMT);
    DESCRIBE_NEXT_UINT(u16, UINT16_MAX, U16_FMT);
    DESCRIBE_NEXT_UINT_BOUNDED(u16, U16_FMT);

    DESCRIBE_NEXT_INT(i8, INT8_MAX, INT8_MIN, I8_FMT);
    DESCRIBE_NEXT_INT_RANGED(i8, I8_FMT);
    DESCRIBE_NEXT_INT(i16, INT16_MAX, INT16_MIN, I16_FMT);
    DESCRIBE_NEXT_INT_RANGED(i16, I16_FMT);
    DESCRIBE_NEXT_INT(i32, INT32_MAX, INT32_MIN, I32_FMT);
    DESCRIBE_NEXT_INT_RANGED(i32, I32_FMT);

    DESCRIBE_NEXT_INT(char, CHAR_MAX, CHAR_MIN, "'%c'");
    DESCRIBE_NEXT_INT_RANGED(char, "'%c'");
    DESCRIBE_NEXT_CHAR_KIND(ascii, "an", "ASCII");
    DESCRIBE_NEXT_CHAR_KIND(print, "a", "printable");
    DESCRIBE_NEXT_CHAR_KIND(lower, "a", "lowercase");
    DESCRIBE_NEXT_CHAR_KIND(upper, "an", "uppercase");
    DESCRIBE_NEXT_CHAR_KIND(alpha, "an", "alphabetic");
    DESCRIBE_NEXT_CHAR_KIND(digit, "a", "numeric");
    DESCRIBE_NEXT_CHAR_KIND(alnum, "an", "alphanumeric");
})
