#include "../bookstore/test.h"

#include "../bookstore/random.h"
#include <time.h>

TEST_MAIN({
    BEFORE_EACH({
        int rounds = 5;
        random_seed(time(NULL) ^ (intptr_t)&printf, (intptr_t)&rounds);
    });

    DESCRIBE("random_next", {
        IT("should generate an unsigned number", {
            u32 n = random_next();
            EXPECT_GTE(n, 0, "%u");
            EXPECT_LTE(n, UINT32_MAX, "%u");
        });
    });

    DESCRIBE("random_next_bounded", {
        IT("should generate a number within the bound", {
            u32 bound = random_next();
            u32 n = random_next_bounded(bound + 1);
            EXPECT_GTE(n, 0, "%u");
            EXPECT_LT(n, bound + 1, "%u");
        });
    });
})
