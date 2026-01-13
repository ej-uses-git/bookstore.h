#include "../bookstore/test.h"

#include "../bookstore/basic.h"
#include "../bookstore/flag.h"

#include <math.h>

const char *flag_error_display(FlagError error) {
    switch (error) {
    case FLAG_NO_ERROR:               return "FLAG_NO_ERROR";
    case FLAG_ERROR_UNKNOWN:          return "FLAG_ERROR_UNKNOWN";
    case FLAG_ERROR_INVALID_BOOL:     return "FLAG_ERROR_INVALID_BOOL";
    case FLAG_ERROR_NO_VALUE:         return "FLAG_ERROR_NO_VALUE";
    case FLAG_ERROR_INVALID_NUMBER:   return "FLAG_ERROR_INVALID_NUMBER";
    case FLAG_ERROR_INTEGER_OVERFLOW: return "FLAG_ERROR_INTEGER_OVERFLOW";
    case FLAG_ERROR_FLOAT_OVERFLOW:   return "FLAG_ERROR_FLOAT_OVERFLOW"; break;
    }
}

const float epsilon = 0.0001;
bool float_eq(float a, float b) {
    return fabs(a - b) < epsilon;
}

#define FLAG "flag"
#define EXPECT_PARSE(arena, args)                                              \
    EXPECT(FLAG_PARSE(arena, args), "failed parsing")
#define EXPECT_PARSE_FAIL(arena, args)                                         \
    EXPECT(!FLAG_PARSE(arena, args), "succeeded parsing")
#define EXPECT_FLAG_ERROR(err)                                                 \
    EXPECT_EQ_MAP(FLAG_GET_ERROR(), err, "%s", flag_error_display)
#define EXPECT_FLAG_ERROR_NAME(flag)                                           \
    EXPECT_SV_EQ(FLAG_GET_ERROR_NAME(), FLAG_NAME(&flag))

TEST_MAIN({
    Arena *arena = arena_new(MiB(1));

    BEFORE_EACH({ arena_clear(arena); });

    DESCRIBE("global flag context", {
        Args args;

        BEFORE_EACH({
            FLAG_INIT(arena);
            FLAG_SET_PROGRAM_NAME(sv_from_cstr("program_name"));
        });

        DESCRIBE("boolean flags", {
            bool flag;

            BEFORE_EACH({ FLAG_BOOL(FLAG, .var = &flag); });

            IT("should parse true when exists", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG);
                EXPECT_PARSE(arena, args);
                EXPECT_TRUE(flag);
            });

            IT("should parse true when \"=true\"", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG "=true");
                EXPECT_PARSE(arena, args);
                EXPECT_TRUE(flag);
            });

            IT("should parse true when \"=1\"", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG "=1");
                EXPECT_PARSE(arena, args);
                EXPECT_TRUE(flag);
            });

            IT("should parse true when \"=yes\"", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG "=yes");
                EXPECT_PARSE(arena, args);
                EXPECT_TRUE(flag);
            });

            IT("should parse true when \"=y\"", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG "=y");
                EXPECT_PARSE(arena, args);
                EXPECT_TRUE(flag);
            });

            IT("should parse false when \"=false\"", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG "=false");
                EXPECT_PARSE(arena, args);
                EXPECT_FALSE(flag);
            });

            IT("should parse false when \"=0\"", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG "=0");
                EXPECT_PARSE(arena, args);
                EXPECT_FALSE(flag);
            });

            IT("should parse false when \"=no\"", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG "=no");
                EXPECT_PARSE(arena, args);
                EXPECT_FALSE(flag);
            });

            IT("should parse false when \"=n\"", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG "=n");
                EXPECT_PARSE(arena, args);
                EXPECT_FALSE(flag);
            });
        });

        DESCRIBE("string flags", {
            StringView flag;
            char *value = "value";

            BEFORE_EACH({ FLAG_STRING(FLAG, .var = &flag); });

            IT("should fail if there's no value", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG);
                EXPECT_PARSE_FAIL(arena, args);
                EXPECT_FLAG_ERROR(FLAG_ERROR_NO_VALUE);
                EXPECT_FLAG_ERROR_NAME(flag);
            });

            IT("should parse from the next argument", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG, value);
                EXPECT_PARSE(arena, args);
                EXPECT_SV_EQ_CSTR(flag, value);
            });

            IT("should parse from an equals sign", {
                args = ARGS_FROM_LIST(sv_printf(arena, "-" FLAG "=%s", value));
                EXPECT_PARSE(arena, args);
                EXPECT_SV_EQ_CSTR(flag, value);
            });
        });

        DESCRIBE("u64 flags", {
            u64 flag;
            u64 value = 1234;

            BEFORE_EACH({ FLAG_U64(FLAG, .var = &flag); });

            IT("should fail if there's no value", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG);
                EXPECT_PARSE_FAIL(arena, args);
                EXPECT_FLAG_ERROR(FLAG_ERROR_NO_VALUE);
                EXPECT_FLAG_ERROR_NAME(flag);
            });

            IT("should fail if there's a non-number value", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG, "value");
                EXPECT_PARSE_FAIL(arena, args);
                EXPECT_FLAG_ERROR(FLAG_ERROR_INVALID_NUMBER);
                EXPECT_FLAG_ERROR_NAME(flag);
            });

            IT("should fail if the value ends in a non-number", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG, "123x");
                EXPECT_PARSE_FAIL(arena, args);
                EXPECT_FLAG_ERROR(FLAG_ERROR_INVALID_NUMBER);
                EXPECT_FLAG_ERROR_NAME(flag);
            });

            IT("should parse from the next argument", {
                args = ARGS_FROM_LIST(sv_from_cstr("-" FLAG),
                                      sv_printf(arena, U64_FMT, value));
                EXPECT_PARSE(arena, args);
                EXPECT_EQ(flag, value, U64_FMT);
            });

            IT("should parse from an equals sign", {
                args = ARGS_FROM_LIST(
                    sv_printf(arena, "-" FLAG "=" U64_FMT, value));
                EXPECT_PARSE(arena, args);
                EXPECT_EQ(flag, value, U64_FMT);
            });

            IT("should parse a trimmed value", {
                args = ARGS_FROM_LIST(
                    sv_printf(arena, "-" FLAG "=  " U64_FMT, value));
                EXPECT_PARSE(arena, args);
                EXPECT_EQ(flag, value, U64_FMT);
            });
        });

        DESCRIBE("float flags", {
            float flag;
            float value = 123.123;

            BEFORE_EACH({ FLAG_FLOAT(FLAG, .var = &flag); });

            IT("should fail if there's no value", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG);
                EXPECT_PARSE_FAIL(arena, args);
                EXPECT_FLAG_ERROR(FLAG_ERROR_NO_VALUE);
                EXPECT_FLAG_ERROR_NAME(flag);
            });

            IT("should fail if there's a non-number value", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG, "value");
                EXPECT_PARSE_FAIL(arena, args);
                EXPECT_FLAG_ERROR(FLAG_ERROR_INVALID_NUMBER);
                EXPECT_FLAG_ERROR_NAME(flag);
            });

            IT("should fail if the value ends in a non-number", {
                args = ARGS_FROM_CSTR_LIST(arena, "-" FLAG, "123x");
                EXPECT_PARSE_FAIL(arena, args);
                EXPECT_FLAG_ERROR(FLAG_ERROR_INVALID_NUMBER);
                EXPECT_FLAG_ERROR_NAME(flag);
            });

            IT("should parse from the next argument", {
                args = ARGS_FROM_LIST(sv_from_cstr("-" FLAG),
                                      sv_printf(arena, "%f", value));
                EXPECT_PARSE(arena, args);
                EXPECT_EQ_FN(flag, value, float_eq, "%f", IDENTITY);
            });
        });
    });

    arena_destroy(arena);
})
