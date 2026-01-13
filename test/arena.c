#include "../bookstore/test.h"

#include "../bookstore/arena.h"

TEST_MAIN({
    i32 size = 4 * sizeof(i32);
    Arena *arena = NULL;

    BEFORE_EACH({
        // Recreate the arena
        arena = arena_new(size);
    });
    AFTER_EACH({
        // Free the underlying memory
        if (arena) arena_destroy(arena);
    });

    DESCRIBE("Arena", {
        DESCRIBE("arena_alloc", {
            IT("should allocate memory from the arena", {
                i32 *buf = arena_alloc(arena, size);
                buf[0] = 0;
                buf[1] = 1;
                buf[2] = 2;
                buf[3] = 3;
            });

            IT_FAIL("asserts that the capacity isn't bypassed", {
                arena_alloc(arena, size + 1);
                UNREACHABLE("should have failed");
            });
        });

        DESCRIBE("arena_clear", {
            IT("should reset the arena's allocations", {
                i32 *buf = arena_alloc(arena, size);
                buf[0] = 0;
                buf[1] = 1;
                buf[2] = 2;
                buf[3] = 3;

                arena_clear(arena);
                EXPECT_EQ(arena->allocated, 0, "%d");

                buf = arena_alloc(arena, size);
                buf[0] = 0;
                buf[1] = 1;
                buf[2] = 2;
                buf[3] = 3;
            });
        });
    });

    DESCRIBE("Lifetime", {
        IT("should allow a temporary lifetime to allocate with", {
            arena_alloc(arena, 1);

            Lifetime lt = lifetime_begin(arena);

            arena_alloc(lt.arena, size - 1);

            EXPECT_EQ(arena->allocated, size, "%d");

            lifetime_end(lt);

            EXPECT_EQ(arena->allocated, 1, "%d");
        });
    });
})
