#include "../bookstore/test.h"

#include "../bookstore/random.h"
#include <ctype.h>
#include <time.h>

TEST_MAIN({
    BEFORE_EACH({
        int rounds = 5;
        random_seed(&rng_global, time(NULL) ^ (intptr_t)&printf,
                    (intptr_t)&rounds);
    });

    DESCRIBE("random_next_u32", {
        IT("should generate an unsigned number", {
            u32 n = random_next_u32(&rng_global);
            EXPECT_LTE(n, UINT32_MAX, "%u");
        });
    });

    DESCRIBE("random_next_u32_bounded", {
        IT("should generate a number within the bound", {
            u32 bound = random_next_u32(&rng_global) + 1;
            u32 n = random_next_u32_bounded(&rng_global, bound);
            EXPECT_LT(n, bound, "%u");
        });
    });

    DESCRIBE("random_next_ascii", {
        IT("should generate an ASCII character", {
            char c = random_next_ascii(&rng_global);
            EXPECTF(isascii(c), "'%c' is not ASCII", c);
        });
    });

    DESCRIBE("random_next_alnum", {
        IT("should generate an alphanumeric character", {
            char c = random_next_alnum(&rng_global);
            EXPECTF(isalnum(c), "'%c' is not alphanumeric", c);
        });
    });
})
