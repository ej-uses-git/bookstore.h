#include "../bookstore/test.h"

#include "../bookstore/hashmap.h"

HASHMAP_TYPEDEF(i32, Hashmap);
HASHMAP_DECLARE_PREFIX(i32, Hashmap, hashmap);

#ifdef BOOKSTORE_IMPLEMENTATION
HASHMAP_DEFINE_PREFIX(i32, Hashmap, hashmap);
#endif // BOOKSTORE_IMPLEMENTATION

StringView generate_key(Arena *arena, Random *rng) {
    i32 count = random_next_u32_bounded(rng, KiB(1));
    StringBuilder sb = sb_new(arena, count);
    for (i32 i = 0; i < count; i++) sb_push(&sb, random_next_print(rng));
    return sb_to_sv(sb);
}

TEST_MAIN({
    Arena *arena = arena_new(MiB(1));

    Lifetime lt;
    BEFORE_EACH({ lt = lifetime_begin(arena); });
    AFTER_EACH({ lifetime_end(lt); });

    DESCRIBE("hashmap_insert", {
        PROP("should return false if the key isn't in the map", rng, {
            Hashmap map = hashmap_new(lt.arena, 1);
            EXPECT_FALSE(hashmap_set(&map, generate_key(lt.arena, &rng),
                                     random_next_i32(&rng)));
        });

        PROP("should set the value for that key in the map", rng, {
            i32 capacity = random_next_i32_ranged(&rng, 1, 128);

            Hashmap map = hashmap_new(lt.arena, capacity * 2);

            for (i32 i = 0; i < capacity - 1; i++) {
                hashmap_set(&map, generate_key(lt.arena, &rng),
                            random_next_i32(&rng));
            }

            StringView key = generate_key(lt.arena, &rng);
            i32 value = random_next_i32(&rng);
            hashmap_set(&map, key, value);

            i32 out;
            EXPECT_TRUE(hashmap_get(map, key, &out));
            EXPECT_EQ(value, out, "%d");
        });

        PROP("should return true if the key is in the map", rng, {
            i32 capacity = random_next_i32_ranged(&rng, 1, 128);

            Hashmap map = hashmap_new(lt.arena, capacity * 2);

            for (i32 i = 0; i < capacity - 1; i++) {
                hashmap_set(&map, generate_key(lt.arena, &rng),
                            random_next_i32(&rng));
            }

            StringView key = generate_key(lt.arena, &rng);
            i32 value = random_next_i32(&rng);
            hashmap_set(&map, key, value);
            EXPECT_TRUE(hashmap_set(&map, key, value));
        });
    });
});
