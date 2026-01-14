#include "../bookstore/test.h"

#include "../bookstore/array.h"

#define BUF_SIZE 4

ARRAY_TYPEDEF(i32, array);
ARRAY_DECLARE(i32, array);
ARRAY_DEFINE(i32, array)

TEST_MAIN({
    i32 arena_size = MiB(2);
    Arena *arena = arena_new(arena_size);

    BEFORE_EACH({ arena_clear(arena); });

    DESCRIBE("array_new", {
        IT("should allocate from an arena and capacity when passed", {
            array arr = array_new(arena, 8);
            array_reserve(&arr, 0);
            EXPECT_EQ((void *)arr.items, (void *)ARENA_MEMORY(arena), "%p");
        });

        IT("should manually allocate when passed NULL", {
            array arr = array_new(NULL, -1);
            EXPECT_NE((void *)arr.items, NULL, "%p");
            free(arr.items);
        });
    });

    DESCRIBE("array_push", {
        IT("should push a new item into the array", {
            array arr = array_new(arena, BUF_SIZE);
            for (i32 i = 0; i < BUF_SIZE; i++) array_push(&arr, i + 1);

            EXPECT_EQ(arr.count, BUF_SIZE, "%d");
            for (i32 i = 0; i < arr.count; i++) {
                EXPECT_EQ(arr.items[i], i + 1, "%d");
            }
        });

        IT_FAIL("should respect the array's capacity if using an arena", {
            array arr = array_new(arena, BUF_SIZE);
            for (i32 i = 0; i <= BUF_SIZE; i++) array_push(&arr, i + 1);
        });

        IT("should increase the array's capacity if dynamic", {
            array arr = array_new(NULL, BUF_SIZE);
            for (i32 i = 0; i <= BUF_SIZE; i++) array_push(&arr, i + 1);
            EXPECT_GT(abs(arr.capacity), BUF_SIZE, "%d");
            free(arr.items);
        });
    });

    DESCRIBE("array_append", {
        IT("should append multiple elements at once", {
            array arr = array_new(arena, BUF_SIZE);
            i32 buf[BUF_SIZE];
            for (i32 i = 0; i < BUF_SIZE; i++) buf[i] = i + 1;

            array_append(&arr, buf, BUF_SIZE);

            EXPECT_EQ(arr.count, BUF_SIZE, "%d");
            for (i32 i = 0; i < arr.count; i++) {
                EXPECT_EQ(arr.items[i], buf[i], "%d");
            }
        });

        IT_FAIL("should respect the array's capacity if using an arena", {
            array arr = array_new(arena, BUF_SIZE);
            i32 buf[BUF_SIZE + 1];
            for (i32 i = 0; i <= BUF_SIZE; i++) buf[i] = i + 1;

            array_append(&arr, buf, BUF_SIZE + 1);
        });

        IT("should increase the array's capacity if dynamic", {
            array arr = array_new(NULL, BUF_SIZE);
            i32 buf[BUF_SIZE + 1];
            for (i32 i = 0; i <= BUF_SIZE; i++) buf[i] = i + 1;

            array_append(&arr, buf, BUF_SIZE + 1);

            EXPECT_GT(abs(arr.capacity), BUF_SIZE, "%d");
            free(arr.items);
        });
    });

    DESCRIBE("array_append_other", {
        IT("should append elements from a different array", {
            array arr = array_new(arena, BUF_SIZE);
            array other = array_new(arena, BUF_SIZE);
            for (i32 i = 0; i < BUF_SIZE; i++) array_push(&other, i + 1);

            array_append_other(&arr, other);

            EXPECT_EQ(arr.count, BUF_SIZE, "%d");
            for (i32 i = 0; i < arr.count; i++) {
                EXPECT_EQ(arr.items[i], other.items[i], "%d");
            }
        });

        IT_FAIL("should respect the array's capacity if using an arena", {
            array arr = array_new(arena, BUF_SIZE);
            array other = array_new(arena, BUF_SIZE + 1);
            for (i32 i = 0; i <= BUF_SIZE; i++) array_push(&other, i + 1);

            array_append_other(&arr, other);
        });

        IT("should increase the array's capacity if dynamic", {
            array arr = array_new(NULL, BUF_SIZE);
            array other = array_new(arena, BUF_SIZE + 1);
            for (i32 i = 0; i <= BUF_SIZE; i++) array_push(&other, i + 1);

            array_append_other(&arr, other);

            EXPECT_GT(abs(arr.capacity), BUF_SIZE, "%d");
            free(arr.items);
        });
    });

    DESCRIBE("array_get", {
        IT("should return the item at the given index", {
            array arr = array_new(arena, BUF_SIZE);
            for (i32 i = 0; i < BUF_SIZE; i++) array_push(&arr, i + 1);
            for (i32 i = 0; i < BUF_SIZE; i++) {
                EXPECT_EQ(array_get(arr, i), i + 1, "%d");
            }
        });

        IT_FAIL("shouldn't allow out of bounds access", {
            array arr = array_new(arena, BUF_SIZE);

            array_get(arr, 0);
        });

        IT("should respect negative indexes", {
            array arr = array_new(arena, BUF_SIZE);
            for (i32 i = 0; i < BUF_SIZE; i++) array_push(&arr, i + 1);

            for (i32 i = BUF_SIZE - 1; i >= 0; i--) {
                i32 index = i - BUF_SIZE;
                i32 value = array_get(arr, index);
                i32 expected = i + 1;
                EXPECT_EQ(value, expected, "%d");
            }
        });
    });

    DESCRIBE("array_pop", {
        IT_FAIL("should fail if the array is empty", {
            array arr = array_new(arena, BUF_SIZE);
            array_pop(&arr);
        });

        IT("should return the last element and decrease the count", {
            array arr = array_new(arena, BUF_SIZE);
            i32 item = 0;
            i32 count = arr.count;
            array_push(&arr, item);
            EXPECT_EQ(item, array_pop(&arr), "%d");
            EXPECT_EQ(arr.count, count, "%d");
        });
    });

    arena_destroy(arena);
})
