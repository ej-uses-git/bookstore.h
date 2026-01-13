#include "../bookstore/test.h"

// TODO: rewrite

TEST_MAIN({
    BEFORE_EACH({ log_info("OVERALL BEFORE EACH"); });
    AFTER_EACH({ log_info("OVERALL AFTER EACH"); });

    DESCRIBE("basic tests", {
        BEFORE_EACH({ log_info("DESCRIBE BEFORE EACH"); });
        AFTER_EACH({ log_info("DESCRIBE AFTER EACH"); });
        IT("part 1", { EXPECT(1 + 1 == 2, "math is wrong"); });
        IT("part 2", { EXPECT(1 + 1 == 2, "math is wrong"); });
    });

    DESCRIBE("other tests", {
        IT("part 1", { EXPECT(1 + 1 == 2, "math is wrong"); });
        IT("part 2", { EXPECT(1 + 1 == 2, "math is wrong"); });
    });
})
