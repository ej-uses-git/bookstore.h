#include "../bookstore/test.h"

#include "../bookstore/slice.h"

#define EXPECT_EQ_D(a, b) EXPECT_EQ(a, b, "%d")

#define BUF_SIZE 10

SLICE_TYPEDEF(i32, slice);
SLICE_DEFINE(i32, slice)

TEST_MAIN({
    i32 buf[BUF_SIZE];
    slice slc;

    BEFORE_EACH({
        for (i32 i = 0; i < BUF_SIZE; i++) buf[i] = i + 1;
        slc = slice_from_parts(buf, BUF_SIZE);
    });

    DESCRIBE("slice_get", {
        IT("should return the item at the given index", {
            for (i32 i = 0; i < BUF_SIZE; i++) {
                EXPECT_EQ_D(slice_get(slc, i), buf[i]);
            }
        });

        IT("should respect negative indexes", {
            for (i32 i = BUF_SIZE - 1; i >= 0; i--) {
                i32 index = i - BUF_SIZE;
                EXPECT_EQ_D(slice_get(slc, index), buf[i]);
            }
        });
    });

    DESCRIBE("slice_shift", {
        IT("should decrease the count by one", {
            i32 count = slc.count;
            slice_shift(&slc);
            EXPECT_EQ_D(slc.count, count - 1);
        });

        IT("should modify where the slice starts", {
            slice_shift(&slc);
            for (i32 i = 0; i < slc.count; i++) {
                EXPECT_EQ_D(slice_get(slc, i), buf[i + 1]);
            }
        });

        IT("should return the first element", {
            i32 chopped = slice_shift(&slc);
            EXPECT_EQ_D(chopped, buf[0]);
        });
    });

    DESCRIBE("slice_pop", {
        IT("should decrease the count by one", {
            slice_pop(&slc);
            EXPECT_EQ_D(slc.count, BUF_SIZE - 1);
        });

        IT("should return the last element", {
            i32 chopped = slice_pop(&slc);
            EXPECT_EQ_D(chopped, buf[BUF_SIZE - 1]);
        });
    });

    DESCRIBE("slice_strip_start", {
        i32 strip_size = BUF_SIZE / 2;
        slice stripped;

        BEFORE_EACH({ stripped = slice_strip_start(&slc, strip_size); });

        IT("should modify the original slice", {
            EXPECT_EQ_D(slc.count, BUF_SIZE - strip_size);
            for (i32 i = 0; i < slc.count; i++) {
                EXPECT_EQ_D(slice_get(slc, i), buf[i + strip_size]);
            }
        });

        IT("should create a new slice with the stripped data", {
            EXPECT_EQ_D(stripped.count, strip_size);
            for (i32 i = 0; i < stripped.count; i++) {
                EXPECT_EQ_D(slice_get(stripped, i), buf[i]);
            }
        });
    });

    DESCRIBE("slice_strip_end", {
        i32 strip_size = BUF_SIZE / 2;
        slice stripped;

        BEFORE_EACH({ stripped = slice_strip_end(&slc, strip_size); });

        IT("should modify the original slice", {
            EXPECT_EQ_D(slc.count, BUF_SIZE - strip_size);
            for (i32 i = 0; i < slc.count; i++) {
                EXPECT_EQ_D(slice_get(slc, i), buf[i]);
            }
        });

        IT("should create a new slice with the stripped data", {
            EXPECT_EQ_D(stripped.count, strip_size);
            for (i32 i = 0; i < stripped.count; i++) {
                EXPECT_EQ_D(slice_get(stripped, i), buf[i + strip_size]);
            }
        });
    });

    DESCRIBE("slice_cut_delimiter_end", {
        slice before;
        i32 target = 5;
        i32 expected_before_count;
        i32 expected_slc_count;

        BEFORE_EACH({
            i32 target_index = slice_index_of(slc, target);
            expected_before_count = target_index < 0 ? slc.count : target_index;
            expected_slc_count =
                target_index < 0 ? 0 : slc.count - target_index - 1;
            before = slice_cut_delimiter_end(&slc, target);
        });

        IT("should modify the original slice to point after the delimiter", {
            EXPECT_LT(slice_index_of(slc, target), 0, "%d");
            EXPECT_EQ_D(slc.count, expected_slc_count);
            for (i32 i = 0; i < slc.count; i++) {
                EXPECT_EQ_D(slice_get(slc, i), buf[i + before.count + 1]);
            }
        });

        IT("should create a new slice to point before the delimiter", {
            EXPECT_LT(slice_index_of(before, target), 0, "%d");
            EXPECT_EQ_D(before.count, expected_before_count);
            for (i32 i = 0; i < before.count; i++) {
                EXPECT_EQ_D(slice_get(before, i), buf[i]);
            }
        });
    });
})
