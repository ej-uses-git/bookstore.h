#include "../bookstore/test.h"

#include "../bookstore/slice.h"

#define EXPECT_EQ_D(a, b) EXPECT_EQ(a, b, I32_FMT)

SLICE_TYPEDEF(i32, Slice);
SLICE_DEFINE_PREFIX(i32, Slice, slice)

#define CAPACITY (KiB(2) * sizeof(i32))

#define SETUP(rng)                                                             \
    i32 buf_size =                                                             \
        random_next_u32_bounded(&rng, (CAPACITY / sizeof(i32)) - 1) + 1;       \
    i32 *buf = arena_alloc(arena, buf_size * sizeof(i32));                     \
    for (i32 i = 0; i < buf_size; i++) buf[i] = random_next_i32(&rng);         \
    Slice slice = slice_from_parts(buf, buf_size)

#define BUF_AND_SLICE_PROP(message, buf, slice, block, ...)                    \
    PROP(message, rng, {                                                       \
        i32 buf##_size =                                                       \
            random_next_u32_bounded(&rng, (CAPACITY / sizeof(i32)) - 1) + 1;   \
        i32 *buf = arena_alloc(arena, buf##_size * sizeof(i32));               \
        for (i32 i = 0; i < buf_size; i++) buf[i] = random_next_i32(&rng);     \
        Slice slice = slice_from_parts(buf, buf##_size);                       \
        do block while (0);                                                    \
    })

TEST_MAIN({
    Arena *arena = arena_new(CAPACITY);

    BEFORE_EACH({ arena_clear(arena); });

    DESCRIBE("slice_get", {
        BUF_AND_SLICE_PROP("should return the given index's item", buf, slice, {
            for (i32 i = 0; i < buf_size; i++) {
                EXPECT_EQ_D(slice_get(slice, i), buf[i]);
            }
        });

        BUF_AND_SLICE_PROP("should respect negative indexes", buf, slice, {
            for (i32 i = buf_size - 1; i >= 0; i--) {
                i32 index = i - buf_size;
                EXPECT_EQ_D(slice_get(slice, index), buf[i]);
            }
        });
    });

    DESCRIBE("slice_shift", {
        BUF_AND_SLICE_PROP("should decrease the count by one", buf, slice, {
            i32 count = slice.count;
            slice_shift(&slice);
            EXPECT_EQ_D(slice.count, count - 1);
        });

        BUF_AND_SLICE_PROP("should modify where the slice starts", buf, slice, {
            slice_shift(&slice);
            for (i32 i = 0; i < slice.count; i++) {
                EXPECT_EQ_D(slice_get(slice, i), buf[i + 1]);
            }
        });

        BUF_AND_SLICE_PROP("should return the first element", buf, slice, {
            i32 chopped = slice_shift(&slice);
            EXPECT_EQ_D(chopped, buf[0]);
        });
    });

    DESCRIBE("slice_pop", {
        BUF_AND_SLICE_PROP("should decrease the count by one", buf, slice, {
            slice_pop(&slice);
            EXPECT_EQ_D(slice.count, buf_size - 1);
        });

        BUF_AND_SLICE_PROP("should return the last element", buf, slice, {
            i32 chopped = slice_pop(&slice);
            EXPECT_EQ_D(chopped, buf[buf_size - 1]);
        });
    });

    DESCRIBE("slice_strip_start", {
        BUF_AND_SLICE_PROP("should modify the original slice", buf, slice, {
            i32 strip_size = random_next_u32_bounded(&rng, buf_size);
            slice_strip_start(&slice, strip_size);

            EXPECT_EQ_D(slice.count, buf_size - strip_size);
            for (i32 i = 0; i < slice.count; i++) {
                EXPECT_EQ_D(slice_get(slice, i), buf[i + strip_size]);
            }
        });

        BUF_AND_SLICE_PROP("should return the stripped data", buf, slice, {
            i32 strip_size = random_next_u32_bounded(&rng, buf_size);
            Slice stripped = slice_strip_start(&slice, strip_size);

            EXPECT_EQ_D(stripped.count, strip_size);
            for (i32 i = 0; i < stripped.count; i++) {
                EXPECT_EQ_D(slice_get(stripped, i), buf[i]);
            }
        });
    });

    DESCRIBE("slice_strip_end", {
        BUF_AND_SLICE_PROP("should modify the original slice", buf, slice, {
            i32 strip_size = random_next_u32_bounded(&rng, buf_size);
            slice_strip_end(&slice, strip_size);

            EXPECT_EQ_D(slice.count, buf_size - strip_size);
            for (i32 i = 0; i < slice.count; i++) {
                EXPECT_EQ_D(slice_get(slice, i), buf[i]);
            }
        });

        BUF_AND_SLICE_PROP("should return the stripped data", buf, slice, {
            i32 strip_size = random_next_u32_bounded(&rng, buf_size);
            Slice stripped = slice_strip_end(&slice, strip_size);

            EXPECT_EQ_D(stripped.count, strip_size);
            for (i32 i = 0; i < stripped.count; i++) {
                EXPECT_EQ_D(slice_get(stripped, i), buf[i + slice.count]);
            }
        });
    });

    DESCRIBE("slice_cut_delimiter_end", {
        BUF_AND_SLICE_PROP(
            "should modify the original to point after the delimiter", buf,
            slice, {
                i32 target = random_next_i32(&rng);
                i32 target_index = slice_index_of(slice, target);
                i32 expected_slice_count =
                    target_index < 0 ? 0 : slice.count - target_index - 1;
                Slice before = slice_cut_delimiter_end(&slice, target);

                EXPECT_LT(slice_index_of(slice, target), 0, I32_FMT);
                EXPECT_EQ_D(slice.count, expected_slice_count);
                for (i32 i = 0; i < slice.count; i++) {
                    EXPECT_EQ_D(slice_get(slice, i), buf[i + before.count + 1]);
                }
            });

        BUF_AND_SLICE_PROP(
            "should return the data before the delimiter", buf, slice, {
                i32 target = random_next_i32(&rng);
                i32 target_index = slice_index_of(slice, target);
                i32 expected_before_count =
                    target_index < 0 ? slice.count : target_index;
                Slice before = slice_cut_delimiter_end(&slice, target);

                EXPECT_LT(slice_index_of(before, target), 0, I32_FMT);
                EXPECT_EQ_D(before.count, expected_before_count);
                for (i32 i = 0; i < before.count; i++) {
                    EXPECT_EQ_D(slice_get(before, i), buf[i]);
                }
            });
    });
})
