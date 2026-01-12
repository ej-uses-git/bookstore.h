/* flag.h */

/*
 * Based heavily on flag.h by Rexim (https://github.com/tsoding/flag.h).
 *
 * Copyright 2021 Alexey Kutepov <reximkut@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef FLAG_H_
#define FLAG_H_

#include "./array.h"
#include "./slice.h"
#include "./string.h"
#include "arena.h"
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#ifndef FLAG_DEFAULT_CAPACITY
#define FLAG_DEFAULT_CAPACITY 24
#endif // FLAG_DEFAULT_CAPACITY

#define PAD_NAME        "    "
#define PAD_DESCRIPTION "          "

// A list of arguments provided over the CLI.
// Use `args_from_main` to convert the arguments to `main` into this type.
SLICE_TYPEDEF(StringView, Args);
SLICE_DECLARE_PREFIX(StringView, Args, args);
// Create an `Args` struct from the arguments passed to `main`, using the
// `arena` to allocate the necessary memory to hold the items.
Args args_from_main(Arena *arena, int argc, const char **argv);
Args args_from_list_null(Arena *arena, ...);
// Create an `Args` struct from the arguments to the macro, which should be
// C-strings (NUL-terminated lists of characters), using the `arena` to allocate
// the necessary memory to hold the items.
//
// Useful for testing command-line applications.
#define ARGS_FROM_CSTR_LIST(arena, ...)                                        \
    args_from_main(arena,                                                      \
                   sizeof((const char *[]){__VA_ARGS__}) / sizeof(char *),     \
                   (const char *[]){__VA_ARGS__})
// Create an `Args` struct from the arguments to the macro, which should be of
// type `StringView`.
//
// Useful for testing command-line applications.
#define ARGS_FROM_LIST(...)                                                    \
    args_from_parts((const StringView[]){__VA_ARGS__},                         \
                    sizeof((StringView[]){__VA_ARGS__}) / sizeof(StringView))

// A context in which flags should be parsed. Whenever it is required, you can
// pass `NULL` to specify the global context.
//
// Multiple contexts are usually necessary for applications with subcommands,
// such that each level of subcommand gets a context of its own (with the
// global context typically being used for the highest level).
typedef struct FlagContext FlagContext;

// Enumeration of the possible errors that can occur when parsing. You can get
// this from a context using `FLAG_GET_ERROR`, and this can then be used to
// manually handle the given errors. However, it is likely that you just need
// the functionality of `FLAG_PRINT_ERROR`, which prints information about the
// error to the given `FILE *` stream.
typedef enum {
    FLAG_NO_ERROR = 0,
    FLAG_ERROR_UNKNOWN,
    FLAG_ERROR_INVALID_BOOL,
    FLAG_ERROR_NO_VALUE,
    FLAG_ERROR_INVALID_NUMBER,
    FLAG_ERROR_INTEGER_OVERFLOW,
    FLAG_ERROR_FLOAT_OVERFLOW,
    // FLAG_ERROR_DOUBLE_OVERFLOW,
    // FLAG_ERROR_INVALID_SIZE_SUFFIX,
} FlagError;

#define FLAG__OPT_TYPEDEF(T, name)                                             \
    typedef struct name {                                                      \
        FlagContext *ctx;                                                      \
        const char *description;                                               \
        const char *alias;                                                     \
        T *var;                                                                \
        T _default;                                                            \
    } name

// The struct of possible options to pass to `flag_bool_opt`, which also act as
// the named optional arguments to the `FLAG_BOOL` macro.
FLAG__OPT_TYPEDEF(bool, FlagBoolOpt);
// Construct a boolean flag with name `name`, explicitly specifying the options
// for initialization as a struct. Returns the pointer to the flag's value.
//
// You may be looking for `FLAG_BOOL`, which allows you to specify only the
// options you need as named optional arguments.
bool *flag_bool_opt(const char *name, FlagBoolOpt opt);
// Construct a boolean flag with name `name`. Returns the pointer to the flag's
// value (this will be equal to the `var` option if you pass it).
//
// You can pass the following named optional arguments to specify additional
// information about the flag:
//
// - `description` - override the description used for `FLAG_PRINT_OPTIONS`
//
//   ```
//   bool help = FLAG_BOOL("help", .description = "Print help and exit.");
//   ```
//
// - `alias` - specify an additional name which can be used for this flag
//
//   ```
//   bool *help = FLAG_BOOL("-help", .alias = "h")
//   ```
//
// - `var` - specify a pointer which should have the flag's value stored in it
//
//   ```
//   bool help;
//   FLAG_BOOL("help", .var = &help);
//   ```
//
// - `_default` - specify a default value for this flag
//
//   ```
//   bool *help = FLAG_BOOL("help", ._default = true);
//   ```
//
// - `ctx` - use a different `FlagContext` than the default
//
//   ```
//   FlagContext *ctx = FLAG_NEW_CONTEXT(arena);
//   bool *help = FLAG_BOOL("help", .ctx = ctx)
//   ```
#define FLAG_BOOL(name, ...) flag_bool_opt(name, (FlagBoolOpt){__VA_ARGS__})

// The struct of possible options to pass to `flag_string_opt`, which also act
// as the named optional arguments to the `FLAG_STRING` macro.
FLAG__OPT_TYPEDEF(StringView, FlagStringOpt);
// Construct a string flag with name `name`, explicitly specifying the options
// for initialization as a struct. Returns the pointer to the flag's value.
//
// You may be looking for `FLAG_STRING`, which allows you to specify only the
// options you need as named optional arguments.
StringView *flag_string_opt(const char *name, FlagStringOpt opt);
// Construct a string flag with name `name`. Returns the pointer to the flag's
// value (this will be equal to the `var` option if you pass it).
//
// You can pass the following named optional arguments to specify additional
// information about the flag:
//
// - `description` - override the description used for `FLAG_PRINT_OPTIONS`
//
//   ```
//   StringView *title = FLAG_STRING("title", .description = "The title.");
//   ```
//
// - `alias` - specify an additional name which can be used for this flag
//
//   ```
//   StringView *title = FLAG_STRING("-title", .alias = "t")
//   ```
//
// - `var` - specify a pointer which should have the flag's value stored in it
//
//   ```
//   StringView title;
//   FLAG_STRING("title", .var = &title);
//   ```
//
// - `_default` - specify a default value for this flag
//
//   ```
//   StringView *title = FLAG_STRING("title", ._default = sv_from_cstr("..."));
//   ```
//
// - `ctx` - use a different `FlagContext` than the default
//
//   ```
//   FlagContext *ctx = FLAG_NEW_CONTEXT(arena);
//   StringView *title = FLAG_STRING("title", .ctx = ctx)
//   ```
#define FLAG_STRING(name, ...)                                                 \
    flag_string_opt(name, (FlagStringOpt){__VA_ARGS__})

// The struct of possible options to pass to `flag_u64_opt`, which also act as
// the named optional arguments to the `FLAG_U64` macro.
FLAG__OPT_TYPEDEF(u64, FlagU64Opt);
// Construct an unsigned integer flag with name `name`, explicitly specifying
// the options for initialization as a struct. Returns the pointer to the flag's
// value.
//
// You may be looking for `FLAG_U64`, which allows you to specify only the
// options you need as named optional arguments.
u64 *flag_u64_opt(const char *name, FlagU64Opt opt);
// Construct an unsigned integer flag with name `name`. Returns the pointer to
// the flag's value (this will be equal to the `var` option if you pass it).
//
// You can pass the following named optional arguments to specify additional
// information about the flag:
//
// - `description` - override the description used for `FLAG_PRINT_OPTIONS`
//
//   ```
//   u64 *amount = FLAG_U64("amount", .description = "The amount.");
//   ```
//
// - `alias` - specify an additional name which can be used for this flag
//
//   ```
//   u64 *amount = FLAG_U64("-amount", .alias = "a")
//   ```
//
// - `var` - specify a pointer which should have the flag's value stored in it
//
//   ```
//   u64 amount;
//   FLAG_U64("amount", .var = &amount);
//   ```
//
// - `_default` - specify a default value for this flag
//
//   ```
//   u64 *amount = FLAG_U64("amount", ._default = 1);
//   ```
//
// - `ctx` - use a different `FlagContext` than the default
//
//   ```
//   FlagContext *ctx = FLAG_NEW_CONTEXT(arena);
//   u64 *amount = FLAG_U64("amount", .ctx = ctx)
//   ```
#define FLAG_U64(name, ...) flag_u64_opt(name, (FlagU64Opt){__VA_ARGS__})

// The struct of possible options to pass to `flag_float_opt`, which also act
// as the named optional arguments to the `FLAG_FLOAT` macro.
FLAG__OPT_TYPEDEF(float, FlagFloatOpt);
float *flag_float_opt(const char *name, FlagFloatOpt opt);
// Construct a floating point flag with name `name`. Returns the pointer to the
// flag's value (this will be equal to the `var` option if you pass it).
//
// You can pass the following named optional arguments to specify additional
// information about the flag:
//
// - `description` - override the description used for `FLAG_PRINT_OPTIONS`
//
//   ```
//   float *seconds = FLAG_FLOAT("seconds",
//                                .description = "How many seconds.");
//   ```
//
// - `alias` - specify an additional name which can be used for this flag
//
//   ```
//   float *seconds = FLAG_FLOAT("-seconds", .alias = "s")
//   ```
//
// - `var` - specify a pointer which should have the flag's value stored in it
//
//   ```
//   float seconds;
//   FLAG_FLOAT("seconds", .var = &seconds);
//   ```
//
// - `_default` - specify a default value for this flag
//
//   ```
//   float *seconds = FLAG_FLOAT("seconds", ._default = 0.5);
//   ```
//
// - `ctx` - use a different `FlagContext` than the default
//
//   ```
//   FlagContext *ctx = FLAG_NEW_CONTEXT(arena);
//   float *seconds = FLAG_FLOAT("seconds", .ctx = ctx)
//   ```
#define FLAG_FLOAT(name, ...) flag_float_opt(name, (FlagFloatOpt){__VA_ARGS__})

// Struct used to specify the amount of flags a context should be able to store,
// used for `flag_init_opt` and `flag_new_context_opt`, and as the named
// optional arguments to `FLAG_INIT` and `FLAG_NEW_CONTEXT`.
//
// NOTE:
// It is recommended that you leave this untouched during development (unless
// you need to increase the default capacity, if you have that many flags...).
// You can then specify it when compiling for production, since you can almost
// always know ahead of time how many flags you are going to create.
typedef struct {
    i32 flag_capacity;
} FlagCapacityOpt;

// Initialize the global flags context, explicitly specifying the options for
// initialization as a struct.
//
// You may be looking for `FLAG_INIT`, which allows you to specify only the
// options you need as named optional arguments.
void flag_init_opt(Arena *arena, FlagCapacityOpt opt);
// Initialize the global flags context, allocating the necessary memory for it
// using `arena`.
//
// You can specify an optional named argument, `flag_capacity`, that specifies
// the amount of flags which the context's internal array can hold.
//
// NOTE:
// It is recommended that you leave `flag_capacity` untouched during development
// (unless you need to increase the default capacity, if you have that many
// flags...). You can then specify it when compiling for production, since you
// can almost always know ahead of time how many flags you are going to create.
#define FLAG_INIT(arena, ...)                                                  \
    flag_init_opt(arena, (FlagCapacityOpt){__VA_ARGS__})

// Create a new flags context, explicitly specifying the options for
// initialization as a struct.
//
// You may be looking for `FLAG_NEW_CONTEXT`, which allows you to specify only
// the options you need as named optional arguments.
FlagContext *flag_new_context_opt(Arena *arena, FlagCapacityOpt opt);
// Create a new flags context, allocating the necessary memory for it
// using `arena`.
//
// You can specify an optional named argument, `flag_capacity`, which customizes
// the amount of flags which the context's internal array can hold.
//
// NOTE:
// It is recommended that you leave `flag_capacity` untouched during development
// (unless you need to increase the default capacity, if you have that many
// flags...). You can then specify it when compiling for production, since you
// can almost always know ahead of time how many flags you are going to create.
#define FLAG_NEW_CONTEXT(arena, ...)                                           \
    flag_new_context_opt(arena, (FlagCapacityOpt){__VA_ARGS__})

typedef struct {
    FlagContext *ctx;
    bool parse_all;
} FlagParseOpt;

// Parse an `Args` struct, explicitly specifying which flags context to populate
// as a struct. Returns a boolean specifying if an error was encountered while
// parsing.
//
// You may be looking for `FLAG_PARSE`, which allows you to specify only the
// options you need as named optional arguments, or even `FLAG_PARSE_MAIN`,
// which allows you to pass the `argc` and `argv` parameters of `main` directly
// into the parsing mechanism.
bool flag_parse_opt(Arena *arena, Args args, FlagParseOpt opt);
// Parse an `Args` struct, populating a given flags context. Uses `arena` to
// allocate memory for the leftover arguments after parsing. Returns a boolean
// specifying if an error was encountered while parsing.
//
// You can specify an optional named argument, `ctx`, that customizes which
// flags context to use. Omitting this option or specifying `NULL` means using
// the global flags context.
#define FLAG_PARSE(arena, args, ...)                                           \
    flag_parse_opt(arena, args, (FlagParseOpt){__VA_ARGS__})
// Parse the `argc` and `argv` parameters of `main`, populating a given flags
// context. Uses `arena` to allocate memory for the `Args` struct and the
// leftover arguments after parsing. Returns a boolean specifying if an error
// was encountered while parsing.
//
// You can specify an optional named argument, `ctx`, that customizes which
// flags context to use. Omitting this option or specifying `NULL` means using
// the global flags context.
#define FLAG_PARSE_MAIN(arena, argc, argv, ...)                                \
    flag_parse_opt(arena, args_from_main(arena, argc, argv),                   \
                   (FlagParseOpt){__VA_ARGS__})

// Struct used to specify which flags context to use for some `flag_*_opt`
// function, and the named optional argument `ctx` for its macro equivalent.
typedef struct {
    FlagContext *ctx;
} FlagContextOpt;

// Get the leftover arguments after parsing, as an `Args` struct, explicitly
// specifying which flags context to use as a struct.
//
// You may be looking for `FLAG_REST_ARGS`, which allows you to specify only the
// options you need as named optional arguments.
Args flag_rest_args_opt(FlagContextOpt opt);
// Get the leftover arguments after parsing, as an `Args` struct.
//
// You can specify an optional named argument, `ctx`, that customizes which
// flags context to use. Omitting this option or specifying `NULL` means using
// the global flags context.
#define FLAG_REST_ARGS(...) flag_rest_args_opt((FlagContextOpt){__VA_ARGS__})

// Set the program name of a flags context, so it doesn't get parsed out of the
// CLI arguments, explicitly specifying which flags context to use as a struct.
//
// You may be looking for `FLAG_SET_PROGRAM_NAME`, which allows you to specify
// only the options you need as named optional arguments.
void flag_set_program_name_opt(StringView name, FlagContextOpt opt);
// Set the program name of a flags context, so it doesn't get parsed out of the
// CLI arguments.
//
// You can specify an optional named argument, `ctx`, that customizes which
// flags context to use. Omitting this option or specifying `NULL` means using
// the global flags context.
#define FLAG_SET_PROGRAM_NAME(name, ...)                                       \
    flag_set_program_name_opt(name, (FlagContextOpt){__VA_ARGS__})

// Get the program name of a flags context, explicitly specifying which flags
// context to use as a struct.
//
// You may be looking for `FLAG_SET_PROGRAM_NAME`, which allows you to specify
// only the options you need as named optional arguments.
StringView flag_get_program_name_opt(FlagContextOpt opt);
// Get the program name of a flags context, as parsed from the CLI or set by
// `FLAG_SET_PROGRAM_NAME`.
//
// You can specify an optional named argument, `ctx`, that customizes which
// flags context to use. Omitting this option or specifying `NULL` means using
// the global flags context.
#define FLAG_GET_PROGRAM_NAME(...)                                             \
    flag_get_program_name_opt((FlagContextOpt){__VA_ARGS__})

// Get the name of a flag by its value within a certain context,
// explicitly specifying which flags context to use as a struct.
//
// You may be looking for `FLAG_NAME`, which allows you to specify
// only the options you need as named optional arguments.
StringView flag_name_opt(void *value, FlagContextOpt opt);
// Get the name of a flag by its value within a certain flags context.
//
// You can specify an optional named argument, `ctx`, that customizes which
// flags context to use. Omitting this option or specifying `NULL` means using
// the global flags context.
#define FLAG_NAME(value, ...)                                                  \
    flag_name_opt(value, (FlagContextOpt){__VA_ARGS__})

// Get the current error in a context, explicitly specifying which flags context
// to use as a struct.
//
// You may be looking for `FLAG_GET_ERROR`, which allows you to specify
// only the options you need as named optional arguments.
FlagError flag_get_error_opt(FlagContextOpt opt);
// Get the current error in a context, for custom error handling. Note that if
// you just want to print some information about the current error, you can use
// `FLAG_PRINT_ERROR`.
//
// You can specify an optional named argument, `ctx`, that customizes which
// flags context to use. Omitting this option or specifying `NULL` means using
// the global flags context.
#define FLAG_GET_ERROR(...) flag_get_error_opt((FlagContextOpt){__VA_ARGS__})

// Get the name of the flag that caused the current error in a context,
// explicitly specifying which flags context to use as a struct.
//
// You may be looking for `FLAG_GET_ERROR_NAME`, which allows you to specify
// only the options you need as named optional arguments.
StringView flag_get_error_name_opt(FlagContextOpt opt);
// Get the name of the flag (possibly an unknown "flag" specified by the CLI
// arguments) that caused the current error in a context, for custom error
// handling. Note that if you just want to print some information about the
// current error, you can use `FLAG_PRINT_ERROR`.
//
// You can specify an optional named argument, `ctx`, that customizes which
// flags context to use. Omitting this option or specifying `NULL` means using
// the global flags context.
#define FLAG_GET_ERROR_NAME(...)                                               \
    flag_get_error_name_opt((FlagContextOpt){__VA_ARGS__})

// Print information about the current error to `stream`, explicitly specifying
// which flags context to use as a struct.
//
// You may be looking for `FLAG_PRINT_ERROR`, which allows you to specify only
// the only the options you need as named optional arguments.
void flag_print_error_opt(FILE *stream, FlagContextOpt opt);
// Print information about the current error to `stream`. You should only call
// this if `FLAG_PARSE` or `FLAG_PARSE_MAIN` returned `false`.
//
// You can specify an optional named argument, `ctx`, that customizes which
// flags context to use. Omitting this option or specifying `NULL` means using
// the global flags context.
#define FLAG_PRINT_ERROR(stream, ...)                                          \
    flag_print_error_opt(stream, (FlagContextOpt){__VA_ARGS__})

// Print information about all the different flags within a context, explicitly
// specifying which flags context to use as a struct.
//
// You may be looking for `FLAG_PRINT_OPTIONS`, which allows you to specify
// only the options you need as named optional arguments.
void flag_print_options_opt(FILE *stream, FlagContextOpt opt);
// Print information about all the different flags within a context.
//
// You can specify an optional named argument, `ctx`, that customizes which
// flags context to use. Omitting this option or specifying `NULL` means using
// the global flags context.
#define FLAG_PRINT_OPTIONS(stream, ...)                                        \
    flag_print_options_opt(stream, (FlagContextOpt){__VA_ARGS__})

#ifdef BOOKSTORE_IMPLEMENTATION

typedef enum {
    FLAG_BOOL,
    FLAG_STRING,
    FLAG_U64,
    FLAG_FLOAT,
} FlagType;

typedef union {
    bool as_bool;
    StringView as_string;
    u64 as_u64;
    float as_float;
} FlagValue;

typedef struct {
    FlagType type;
    StringView name;
    StringView alias;
    StringView description;

    FlagValue value;
    void *ref;

    FlagValue _default;
} Flag;

ARRAY_TYPEDEF(Flag, FlagArray);
ARRAY_DEFINE_PREFIX(Flag, FlagArray, flag_array)

ARRAY_TYPEDEF(StringView, ArgsBuilder);
ARRAY_DEFINE_PREFIX(StringView, ArgsBuilder, ab)

struct FlagContext {
    FlagArray flags;
    ArgsBuilder ab;
    StringView program_name;
    StringView error_name;
    FlagError error;
};

global FlagContext flag_global_context;

SLICE_DEFINE_COMPLEX_PREFIX(StringView, sv_eq, Args, args)

Args args_from_main(Arena *arena, int argc, const char **argv) {
    ArgsBuilder ab = ab_new(arena, argc);
    for (i32 i = 0; i < argc; i++) ab_push(&ab, sv_from_cstr(argv[i]));
    return args_from_parts(ab.items, ab.count);
}

internal Flag *flag__new(FlagContext *c, FlagType type, const char *name,
                         const char *alias, const char *description) {
    Flag flag = {0};
    flag.type = type;
    flag.name = sv_from_cstr(name);
    flag.description = sv_from_cstr(description);
    flag.alias = alias ? sv_from_cstr(alias) : SV_EMPTY;
    flag_array_push(&c->flags, flag);
    return flag_array_get_ref(c->flags, -1);
}

internal void *flag__get_ref(Flag *flag) {
    if (flag->ref) return flag->ref;
    return &flag->value;
}

internal void flag__init_context(FlagContext *c, Arena *arena,
                                 i32 flag_capacity) {
    memset(c, 0, sizeof(FlagContext));
    if (!flag_capacity) {
        flag_capacity = FLAG_DEFAULT_CAPACITY;
    }
    c->flags = flag_array_new(arena, flag_capacity);
}

void flag_init_opt(Arena *arena, FlagCapacityOpt opt) {
    flag__init_context(&flag_global_context, arena, opt.flag_capacity);
}

FlagContext *flag_new_context_opt(Arena *arena, FlagCapacityOpt opt) {
    FlagContext *c = arena_alloc(arena, sizeof(FlagContext));
    flag__init_context(c, arena, opt.flag_capacity);
    return c;
}

bool *flag_bool_opt(const char *name, FlagBoolOpt opt) {
    FlagContext *c = opt.ctx ? opt.ctx : &flag_global_context;

    Flag *flag =
        flag__new(c, FLAG_BOOL, name, opt.alias,
                  opt.description ? opt.description : "A boolean value.");
    flag->_default.as_bool = opt._default;
    if (opt.var) {
        flag->ref = opt.var;
        *(opt.var) = opt._default;
        return opt.var;
    } else {
        flag->value.as_bool = opt._default;
        return &flag->value.as_bool;
    }
}

StringView *flag_string_opt(const char *name, FlagStringOpt opt) {
    FlagContext *c = opt.ctx ? opt.ctx : &flag_global_context;

    Flag *flag =
        flag__new(c, FLAG_STRING, name, opt.alias,
                  opt.description ? opt.description : "A string value.");
    flag->_default.as_string = opt._default;
    if (opt.var) {
        flag->ref = opt.var;
        *(opt.var) = opt._default;
        return opt.var;
    } else {
        flag->value.as_string = opt._default;
        return &flag->value.as_string;
    }
}

u64 *flag_u64_opt(const char *name, FlagU64Opt opt) {
    FlagContext *c = opt.ctx ? opt.ctx : &flag_global_context;

    Flag *flag = flag__new(c, FLAG_U64, name, opt.alias,
                           opt.description ? opt.description :
                                             "An unsigned integer value.");
    flag->_default.as_u64 = opt._default;
    if (opt.var) {
        flag->ref = opt.var;
        *(opt.var) = opt._default;
        return opt.var;
    } else {
        flag->value.as_u64 = opt._default;
        return &flag->value.as_u64;
    }
}

float *flag_float_opt(const char *name, FlagFloatOpt opt) {
    FlagContext *c = opt.ctx ? opt.ctx : &flag_global_context;

    Flag *flag =
        flag__new(c, FLAG_FLOAT, name, opt.alias,
                  opt.description ? opt.description : "A float value.");
    flag->_default.as_float = opt._default;
    if (opt.var) {
        flag->ref = opt.var;
        *(opt.var) = opt._default;
        return opt.var;
    } else {
        flag->value.as_float = opt._default;
        return &flag->value.as_float;
    }
}

bool flag_parse_opt(Arena *arena, Args args, FlagParseOpt opt) {
    FlagContext *c = opt.ctx ? opt.ctx : &flag_global_context;

    if (c->ab.capacity < args.count) {
        c->ab = ab_new(arena, args.count);
    } else {
        c->ab.count = 0;
    }

    if (c->program_name.count == 0) {
        c->program_name = args_shift(&args);
    }

    bool all_args = false;
    while (args.count) {
        StringView arg = args_shift(&args);

        if (all_args || sv_get(arg, 0) != '-') {
            if (!opt.parse_all) {
                args.count += 1;
                args.data -= 1;
                ab_append(&c->ab, args.data, args.count);
                return true;
            } else {
                ab_push(&c->ab, arg);
                continue;
            }
        }

        if (sv_eq_cstr(arg, "--")) {
            if (!opt.parse_all) {
                ab_append(&c->ab, args.data, args.count);
                return true;
            } else {
                all_args = true;
                continue;
            }
        }

        sv_shift(&arg);

        bool ignore = false;
        if (sv_get(arg, 0) == '/') {
            ignore = true;
            sv_shift(&arg);
        }

        StringView equals = arg;
        arg = sv_cut_delimiter(&equals, '=');

        bool found = false;
        for (i32 i = 0; i < c->flags.count; i++) {
            Flag *flag = flag_array_get_ref(c->flags, i);
            if (sv_eq(flag->name, arg) ||
                (flag->alias.count && sv_eq(flag->alias, arg))) {
                found = true;
                switch (flag->type) {
                case FLAG_BOOL: {
                    bool value;
                    if (!equals.count) {
                        value = true;
                    } else if (sv_eq_cstr(equals, "on") ||
                               sv_eq_cstr(equals, "true") ||
                               sv_eq_cstr(equals, "yes") ||
                               sv_eq_cstr(equals, "y") ||
                               sv_eq_cstr(equals, "1")) {
                        value = true;
                    } else if (sv_eq_cstr(equals, "off") ||
                               sv_eq_cstr(equals, "false") ||
                               sv_eq_cstr(equals, "no") ||
                               sv_eq_cstr(equals, "n") ||
                               sv_eq_cstr(equals, "0")) {
                        value = false;
                    } else {
                        c->error = FLAG_ERROR_INVALID_BOOL;
                        c->error_name = arg;
                        return false;
                    }

                    if (!ignore) *(bool *)flag__get_ref(flag) = value;
                } break;
                case FLAG_STRING: {
                    StringView value;
                    if (!equals.count) {
                        if (!args.count) {
                            c->error = FLAG_ERROR_NO_VALUE;
                            c->error_name = arg;
                            return false;
                        }

                        value = args_shift(&args);
                    } else {
                        value = equals;
                    }

                    if (!ignore) *(StringView *)flag__get_ref(flag) = value;
                } break;
                case FLAG_U64: {
                    StringView value;
                    if (!equals.count) {
                        if (!args.count) {
                            c->error = FLAG_ERROR_NO_VALUE;
                            c->error_name = arg;
                            return false;
                        }

                        value = args_shift(&args);
                    } else {
                        value = equals;
                    }

                    u64 result = sv_parse_u64(&value, 10);
                    log_debug(U64_FMT, result);
                    log_debug("'" SV_FMT "'", SV_ARG(value));

                    if (result == UINT64_MAX) {
                        c->error = FLAG_ERROR_INTEGER_OVERFLOW;
                        c->error_name = arg;
                        return false;
                    }

                    if (value.count) {
                        c->error = FLAG_ERROR_INVALID_NUMBER;
                        c->error_name = arg;
                        return false;
                    }

                    if (!ignore) *(u64 *)flag__get_ref(flag) = result;
                } break;
                case FLAG_FLOAT: {
                    StringView value;
                    if (!equals.count) {
                        if (!args.count) {
                            c->error = FLAG_ERROR_NO_VALUE;
                            c->error_name = arg;
                            return false;
                        }

                        value = args_shift(&args);
                    } else {
                        value = equals;
                    }

                    errno = 0;
                    float result = sv_parse_float(&value);

                    if (errno == ERANGE) {
                        c->error = FLAG_ERROR_FLOAT_OVERFLOW;
                        c->error_name = arg;
                        return false;
                    }

                    if (value.count) {
                        c->error = FLAG_ERROR_INVALID_NUMBER;
                        c->error_name = arg;
                        return false;
                    }

                    if (!ignore) *(float *)flag__get_ref(flag) = result;
                } break;
                }
            }
        }

        if (!found) {
            c->error = FLAG_ERROR_UNKNOWN;
            c->error_name = arg;
            return false;
        }
    }

    ab_append(&c->ab, args.data, args.count);
    return true;
}

Args flag_rest_args_opt(FlagContextOpt opt) {
    FlagContext *c = opt.ctx ? opt.ctx : &flag_global_context;
    return args_from_parts(c->ab.items, c->ab.count);
}

void flag_set_program_name_opt(StringView name, FlagContextOpt opt) {
    FlagContext *c = opt.ctx ? opt.ctx : &flag_global_context;

    c->program_name = name;
}

StringView flag_get_program_name_opt(FlagContextOpt opt) {
    FlagContext *c = opt.ctx ? opt.ctx : &flag_global_context;
    return c->program_name;
}

StringView flag_name_opt(void *value, FlagContextOpt opt) {
    FlagContext *c = opt.ctx ? opt.ctx : &flag_global_context;

    for (i32 i = 0; i < c->flags.count; i++) {
        Flag *flag = flag_array_get_ref(c->flags, i);
        if (flag__get_ref(flag) == value) {
            return flag->name;
        }
    }

    return SV_EMPTY;
}

FlagError flag_get_error_opt(FlagContextOpt opt) {
    FlagContext *c = opt.ctx ? opt.ctx : &flag_global_context;
    return c->error;
}

StringView flag_get_error_name_opt(FlagContextOpt opt) {
    FlagContext *c = opt.ctx ? opt.ctx : &flag_global_context;
    return c->error_name;
}

void flag_print_error_opt(FILE *stream, FlagContextOpt opt) {
    FlagContext *c = opt.ctx ? opt.ctx : &flag_global_context;
    switch (c->error) {
    case FLAG_NO_ERROR:
        fprintf(stream,
                "Whoops! You seem to have called `flag_print_error` without "
                "`FLAG_PARSE` having failed!\nMaybe you have mismatched "
                "`FlagContext`s?\n");
        break;
    case FLAG_ERROR_UNKNOWN:
        fprintf(stream, "ERROR: unknown flag '" SV_FMT "'\n",
                SV_ARG(c->error_name));
        break;
    case FLAG_ERROR_INVALID_BOOL:
        fprintf(stream, "ERROR: -" SV_FMT ": invalid boolean value\n",
                SV_ARG(c->error_name));
        break;
    case FLAG_ERROR_NO_VALUE:
        fprintf(stream, "ERROR: -" SV_FMT ": no value provided\n",
                SV_ARG(c->error_name));
        break;
    case FLAG_ERROR_INVALID_NUMBER:
        fprintf(stream, "ERROR: -" SV_FMT ": invalid number\n",
                SV_ARG(c->error_name));
        break;
    case FLAG_ERROR_INTEGER_OVERFLOW:
        fprintf(stream, "ERROR: -" SV_FMT ": integer overflow\n",
                SV_ARG(c->error_name));
        break;
    case FLAG_ERROR_FLOAT_OVERFLOW:
        fprintf(stream, "ERROR: -" SV_FMT ": float overflow\n",
                SV_ARG(c->error_name));
        break;
    }
}

void flag_print_options_opt(FILE *stream, FlagContextOpt opt) {
    FlagContext *c = opt.ctx ? opt.ctx : &flag_global_context;
    for (i32 i = 0; i < c->flags.count; i++) {
        Flag flag = flag_array_get(c->flags, i);
        switch (flag.type) {
        case FLAG_BOOL:
            fprintf(stream, PAD_NAME "-" SV_FMT, SV_ARG(flag.name));
            if (flag.alias.count) {
                fprintf(stream, ", -" SV_FMT, SV_ARG(flag.alias));
            }
            fprintf(stream, "\n");
            fprintf(stream,
                    PAD_DESCRIPTION SV_FMT "\n" PAD_DESCRIPTION "Default: %s\n",
                    SV_ARG(flag.description),
                    flag._default.as_bool ? "true" : "false");
            break;
        case FLAG_STRING:
            fprintf(stream, PAD_NAME "-" SV_FMT, SV_ARG(flag.name));
            if (flag.alias.count) {
                fprintf(stream, ", -" SV_FMT, SV_ARG(flag.alias));
            }
            fprintf(stream, " <str>\n");
            fprintf(stream, PAD_DESCRIPTION SV_FMT "\n",
                    SV_ARG(flag.description));
            if (flag._default.as_string.count) {
                fprintf(stream, PAD_DESCRIPTION "Default: \"" SV_FMT "\"\n",
                        SV_ARG(flag._default.as_string));
            }
            break;
        case FLAG_U64:
            fprintf(stream, PAD_NAME "-" SV_FMT, SV_ARG(flag.name));
            if (flag.alias.count) {
                fprintf(stream, ", -" SV_FMT, SV_ARG(flag.alias));
            }
            fprintf(stream, " <uint>\n");
            fprintf(stream, PAD_DESCRIPTION SV_FMT "\n",
                    SV_ARG(flag.description));
            if (flag._default.as_u64) {
                fprintf(stream, PAD_DESCRIPTION "Default: " U64_FMT "\n",
                        flag._default.as_u64);
            }
            break;
        case FLAG_FLOAT:
            fprintf(stream, PAD_NAME "-" SV_FMT, SV_ARG(flag.name));
            if (flag.alias.count) {
                fprintf(stream, ", -" SV_FMT, SV_ARG(flag.alias));
            }
            fprintf(stream, " <float>\n");
            fprintf(stream, PAD_DESCRIPTION SV_FMT "\n",
                    SV_ARG(flag.description));
            if (flag._default.as_u64) {
                fprintf(stream, PAD_DESCRIPTION "Default: %f\n",
                        flag._default.as_float);
            }
            break;
        }
        fprintf(stream, "\n");
    }
}

#endif // BOOKSTORE_IMPLEMENTATION

#endif // FLAG_H_
