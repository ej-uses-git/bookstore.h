#include "../bookstore/test.h"

#include "../bookstore/aatree.h"
#include "../bookstore/random.h"

#include <time.h>

AANODE_TYPEDEF(i32, Node);
AATREE_TYPEDEF(Node, Tree);
AATREE_DECLARE_PREFIX(i32, Node, Tree, tree);

AATREE_DEFINE_PREFIX(i32, Node, Tree, tree)

bool tree_visit(TreeWalkEntry entry) {
    i32 *last = entry.user_data;

    EXPECT_GTE(entry.value, *last, "%d");

    *last = entry.value;

    return true;
}

TEST_MAIN({
    Arena *arena = arena_new(MiB(1));
    Lifetime lt;

    BEFORE_EACH({
        lt = lifetime_begin(arena);

        int rounds = 5;
        random_seed(time(NULL) ^ (intptr_t)&printf, (intptr_t)&rounds);
    });

    AFTER_EACH({ lifetime_end(lt); });

    DESCRIBE("tree_insert", {
        IT_FAIL("should fail if over capacity", {
            Tree t = tree_new(lt.arena, 0);
            tree_insert(lt.arena, &t, random_next());
            UNREACHABLE("should have failed earlier");
        });

        IT("should return false if the value isn't in the tree", {
            Tree t = tree_new(lt.arena, 1);
            EXPECT_FALSE(tree_insert(lt.arena, &t, random_next()));
        });

        IT("should insert the value into the tree", {
            Tree t = tree_new(lt.arena, 1);
            i32 value = random_next();
            tree_insert(lt.arena, &t, value);
            EXPECT_NON_NULL(tree_find(lt.arena, t, value));
        });

        IT("should return true if the value is the tree", {
            Tree t = tree_new(lt.arena, 2);
            i32 value = random_next();
            tree_insert(lt.arena, &t, value);
            EXPECT_TRUE(tree_insert(lt.arena, &t, value));
        });
    });

    DESCRIBE("tree_delete", {
        IT("should return false if the value isn't in the tree", {
            Tree t = tree_new(lt.arena, 0);
            EXPECT_FALSE(tree_delete(lt.arena, &t, random_next()));
        });

        IT("should return true if the value is in the tree", {
            Tree t = tree_new(lt.arena, 1);
            i32 value = random_next();
            tree_insert(lt.arena, &t, value);
            EXPECT_NON_NULL(tree_delete(lt.arena, &t, value));
        });

        IT("should remove the value from the tree", {
            i32 count = random_next_bounded(64) + 1;

            Tree t = tree_new(lt.arena, count);
            for (i32 i = 0; i < count - 1; i++)
                tree_insert(lt.arena, &t, random_next());

            i32 value = random_next();
            tree_insert(lt.arena, &t, value);
            tree_delete(lt.arena, &t, value);
            EXPECT_NULL(tree_find(lt.arena, t, value));
        });
    });

    DESCRIBE("tree_find", {
        IT("should return NULL if the value isn't in the tree", {
            Tree t = tree_new(lt.arena, 0);
            EXPECT_NULL(tree_find(lt.arena, t, random_next()));
        });

        IT("should return a pointer to the value if it is in the tree", {
            Tree t = tree_new(lt.arena, 1);
            i32 value = random_next();
            tree_insert(lt.arena, &t, value);
            EXPECT_NON_NULL(tree_find(lt.arena, t, value));
        });
    });

    DESCRIBE("tree_walk", {
        IT("should traverse the values in order", {
            i32 count = random_next_bounded(64) + 1;

            Tree t = tree_new(lt.arena, count);

            for (i32 i = 0; i < count; i++)
                tree_insert(lt.arena, &t, random_next());

            i32 min = 0;
            tree_walk(lt.arena, t, tree_visit, &min);
        });
    });
})
