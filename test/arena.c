#include "../bookstore/test.h"

#include "../bookstore/arena.h"

TEST_MAIN({
    Arena *arena = NULL;

    AFTER_EACH({
        // Free the underlying memory
        if (arena) arena_destroy(arena);
    });

    DESCRIBE("arena_alloc", {
        PROP("should allocate memory from the arena", rng, {
            i32 size = random_next_i32_ranged(&rng, 0, KiB(2));
            arena = arena_new(size * sizeof(i32));

            i32 *buf = arena_alloc(arena, size * sizeof(i32));
            for (i32 i = 0; i < size; i++) buf[i] = i;
        });

        PROP_FAIL("asserts that the capacity isn't bypassed", rng, {
            i32 size = random_next_i32_ranged(&rng, 0, INT32_MAX);
            arena = arena_new(size);
            arena_alloc(arena, size + 1);
        });
    });

    DESCRIBE("arena_clear", {
        PROP("should reset the arena's allocations", rng, {
            i32 size = random_next_i32_ranged(&rng, 0, KiB(2));
            arena = arena_new(size * sizeof(i32));

            i32 *buf = arena_alloc(arena, size * sizeof(i32));
            for (i32 i = 0; i < size; i++) buf[i] = i;

            arena_clear(arena);
            EXPECT_EQ(arena->allocated, 0, I32_FMT);

            buf = arena_alloc(arena, size * sizeof(i32));
            for (i32 i = 0; i < size; i++) buf[i] = i;
        });
    });

    DESCRIBE("lifetime_begin/lifetime_end", {
        PROP("should create/end a temporary allocation lifetime", rng, {
            i32 size = random_next_i32_ranged(&rng, 0, KiB(2));
            arena = arena_new(size * sizeof(i32));

            arena_alloc(arena, 1);

            Lifetime lt = lifetime_begin(arena);

            arena_alloc(lt.arena, size - 1);

            EXPECT_EQ(arena->allocated, size, I32_FMT);

            lifetime_end(lt);

            EXPECT_EQ(arena->allocated, 1, I32_FMT);
        });
    });
})
