#ifndef JSON_H_
#define JSON_H_

#include "./arena.h"
#include "./array.h"
#include "./string.h"
#include <limits.h>
#include <stdarg.h>

typedef struct JsonParser JsonParser;

typedef struct {
    i32 max_depth;
} JsonParserNewOpt;

JsonParser *jp_new_opt(Arena *arena, StringView source, JsonParserNewOpt opt);
#define JP_NEW(arena, source, ...)                                             \
    jp_new_opt(arena, source, (WRAPPER(JsonParserNewOpt){__VA_ARGS__}).wrapper)
void jp_errorf(JsonParser *jp, const char *fmt, ...) PRINTF_FORMAT(2, 3);
bool jp_float(JsonParser *jp, float *out);
bool jp_string(JsonParser *jp, StringBuilder *out);
bool jp_string_append(JsonParser *jp, StringBuilder *out);
bool jp_bool(JsonParser *jp, bool *out);
bool jp_null(JsonParser *jp);
bool jp_array_begin(JsonParser *jp);
bool jp_array_end(JsonParser *jp);
bool jp_object_begin(JsonParser *jp);
bool jp_object_end(JsonParser *jp);
bool jp_key(JsonParser *jp, StringBuilder *out);
bool jp_key_append(JsonParser *jp, StringBuilder *out);
StringView jp_get_error(Arena *arena, JsonParser *jp);
void jp_print_error(JsonParser *jp, FILE *stream);

typedef struct JsonBuilder JsonBuilder;

typedef struct {
    i32 max_depth;
    i32 padding;
} JsonBuilderNewOpt;

JsonBuilder *jb_new_opt(Arena *arena, i32 capacity, JsonBuilderNewOpt opt);
#define JB_NEW(arena, capacity, ...)                                           \
    jb_new_opt(arena, capacity,                                                \
               ((WRAPPER(JsonBuilderNewOpt){__VA_ARGS__}).wrapper))
void jb_begin(JsonBuilder *jb);
void jb_float(JsonBuilder *jb, float value);
void jb_integer(JsonBuilder *jb, i32 value);
void jb_string_view(JsonBuilder *jb, StringView value);
void jb_string_begin(JsonBuilder *jb);
void jb_string_append(JsonBuilder *jb, StringView value);
void jb_string_push(JsonBuilder *jb, char c);
#define JB_STRING(jb, block)                                                   \
    do {                                                                       \
        jb_string_begin(jb);                                                   \
        do block while (0);                                                    \
        jb_string_end(jb);                                                     \
    } while (0)
void jb_string_end(JsonBuilder *jb);
void jb_bool(JsonBuilder *jb, bool value);
void jb_null(JsonBuilder *jb);
void jb_array_begin(JsonBuilder *jb);
void jb_array_end(JsonBuilder *jb);
#define JB_ARRAY(jb, block)                                                    \
    do {                                                                       \
        jb_array_begin(jb);                                                    \
        do block while (0);                                                    \
        jb_array_end(jb);                                                      \
    } while (0)
void jb_object_begin(JsonBuilder *jb);
void jb_object_end(JsonBuilder *jb);
#define JB_OBJECT(jb, block)                                                   \
    do {                                                                       \
        jb_object_begin(jb);                                                   \
        do block while (0);                                                    \
        jb_object_end(jb);                                                     \
    } while (0)
void jb_key(JsonBuilder *jb, StringView key);
void jb_key_cstr(JsonBuilder *jb, const char *key);
#define JB_KEY_LITERAL(jb, key) jb_key(jb, SV_FROM_LITERAL(key))
StringView jb_end(JsonBuilder *jb);

#ifdef BOOKSTORE_IMPLEMENTATION

typedef enum {
    JSON_STATE_OBJECT_BEGIN,
    JSON_STATE_OBJECT,
    JSON_STATE_KEY,
    JSON_STATE_ARRAY_BEGIN,
    JSON_STATE_ARRAY,
    JSON_STATE_STRING,
} Json__State;

ARRAY_TYPEDEF(Json__State, Json__StateStack);
ARRAY_DEFINE(Json__State, Json__StateStack);

struct JsonParser {
    const char *initial;
    StringView source;
    Json__StateStack stack;
    StringBuilder error;
    i32 error_char;
};

bool jp__non_eof(JsonParser *jp, char expected, StringView mark) {
    if (!jp->source.count) {
        jp_errorf(jp, "expected '%c', received EOF", expected);
        jp->source = mark;
        return false;
    }
    return true;
}

bool jp__expect(JsonParser *jp, char expected, StringView mark) {
    if (!jp__non_eof(jp, expected, mark)) return false;

    char received = sv_shift(&jp->source);
    if (received != expected) {
        jp_errorf(jp, "expected '%c', received '%c'", expected, received);
        jp->source = mark;
        return false;
    }

    return true;
}

bool jp__value_handle_state(JsonParser *jp, StringView mark) {
    sv_trim_start(&jp->source);

    if (!jp->stack.count) return true;

    // TODO: multiple values?
    // if (jp->stack.count < 0) {
    //     ASSERT(false,
    //            "Cannot render two values into the same builder."
    //            "You may want to reset the builder after using the result.");
    // }

    Json__State *state_ref = Json__StateStack_get_ref(jp->stack, -1);

    switch (*state_ref) {
    case JSON_STATE_OBJECT_BEGIN: // fallthrough
    case JSON_STATE_OBJECT:       {
        ASSERT(false,
               "Cannot parse a value directly from an object;"
               "did you forget to call `jp_key`?");
    } break;
    case JSON_STATE_KEY: {
        Json__StateStack_pop(&jp->stack);
    } break;
    case JSON_STATE_ARRAY_BEGIN: {
        *state_ref = JSON_STATE_ARRAY;
    } break;
    case JSON_STATE_ARRAY: {
        if (!jp__expect(jp, ',', mark)) return false;
        sv_trim_start(&jp->source);
    } break;
    case JSON_STATE_STRING: {
        UNREACHABLE("ಠ╭╮ಠ Have you been screwing with `jp->stack`?");
    } break;
    }
    return true;
}

bool jp__key_handle_state(JsonParser *jp, StringView mark) {
    sv_trim_start(&jp->source);

    ASSERT(jp->stack.count > 0,
           "Cannot parse a key outside an object;"
           "did you forget to call `jp_object_begin`?");

    Json__State *state_ref = Json__StateStack_get_ref(jp->stack, -1);

    switch (*state_ref) {
    case JSON_STATE_OBJECT_BEGIN: {
        *state_ref = JSON_STATE_OBJECT;
    } break;
    case JSON_STATE_OBJECT: {
        if (!jp__expect(jp, ',', mark)) return false;
        sv_trim_start(&jp->source);
    } break;
    case JSON_STATE_KEY: {
        ASSERT(false,
               "Cannot parse a key twice in a row;"
               "did you forget to parse a value after calling `jp_key`?");
    } break;
    case JSON_STATE_ARRAY_BEGIN: // fallthrough
    case JSON_STATE_ARRAY:       {
        ASSERT(false,
               "Cannot parse a key inside an array;"
               "did you forget to call `jp_object_begin`?");
    } break;
    case JSON_STATE_STRING: {
        UNREACHABLE("ಠ╭╮ಠ Have you been screwing with `jp->stack`?");
    } break;
    }

    return true;
}

bool jp__parse_string(JsonParser *jp, StringBuilder *out, StringView mark) {
    while (jp->source.count && sv_get(jp->source, 0) != '"') {
        char c = sv_shift(&jp->source);
        if (c == '\\') {
            if (!jp__non_eof(jp, '"', mark)) return false;
            c = sv_shift(&jp->source);
            switch (c) {
            case 'n': sb_push(out, '\n'); break;
            case 'r': sb_push(out, '\r'); break;
            case 't': sb_push(out, '\t'); break;
            default:  sb_push(out, c); break;
            }
        } else {
            sb_push(out, c);
        }
    }
    if (!jp__non_eof(jp, '"', mark)) return false;
    return true;
}

JsonParser *jp_new_opt(Arena *arena, StringView source, JsonParserNewOpt opt) {
    JsonParser *jp = arena_alloc(arena, sizeof(JsonParser));
    if (!opt.max_depth) opt.max_depth = 64;
    jp->stack = Json__StateStack_new(arena, opt.max_depth);
    jp->source = source;
    jp->initial = source.data;
    jp->error = sb_new(arena, 128);
    jp->error_char = 0;
    return jp;
}

void jp_errorf(JsonParser *jp, const char *fmt, ...) {
    jp->error.count = 0;
    jp->error_char = jp->source.data - jp->initial;

    va_list args;

    va_start(args, fmt);
    sb_vappendf(&jp->error, fmt, args);
    va_end(args);
}

bool jp_float(JsonParser *jp, float *out) {
    StringView mark = jp->source;
    if (!jp__value_handle_state(jp, mark)) return false;

    i32 start = jp->source.count;
    *out = sv_parse_float(&jp->source);
    if (start == jp->source.count) {
        jp->error.count = 0;
        sb_append_sv(&jp->error,
                     SV_FROM_LITERAL("expected a digit, received "));
        if (jp->source.count) {
            sb_appendf(&jp->error, "'%c'", sv_get(jp->source, 0));
        } else {
            sb_append_sv(&jp->error, SV_FROM_LITERAL("EOF"));
        }
        jp->error_char = (jp->source.data - jp->initial) + 1;
        jp->source = mark;
        return false;
    }
    return true;
}

bool jp_string(JsonParser *jp, StringBuilder *out) {
    out->count = 0;
    return jp_string_append(jp, out);
}

bool jp_string_append(JsonParser *jp, StringBuilder *out) {
    StringView mark = jp->source;
    if (!jp__value_handle_state(jp, mark)) return false;

    if (!jp__expect(jp, '"', mark)) return false;
    if (!jp__parse_string(jp, out, mark)) return false;
    if (!jp__expect(jp, '"', mark)) return false;
    return true;
}

const StringView JSON__TRUE = SV_FROM_LITERAL("true");
const StringView JSON__FALSE = SV_FROM_LITERAL("false");
const StringView JSON__NULL = SV_FROM_LITERAL("null");

bool jp_bool(JsonParser *jp, bool *out) {
    if (!jp__value_handle_state(jp, jp->source)) return false;

    if (sv_strip_prefix(&jp->source, JSON__TRUE)) {
        *out = true;
    } else if (sv_strip_prefix(&jp->source, JSON__FALSE)) {
        *out = false;
    } else {
        jp->error.count = 0;
        sb_append_sv(&jp->error,
                     SV_FROM_LITERAL("expected true or false, received "));
        if (jp->source.count) {
            sb_appendf(&jp->error, "'%c'", sv_get(jp->source, 0));
        } else {
            sb_append_sv(&jp->error, SV_FROM_LITERAL("EOF"));
        }
        jp->error_char = jp->source.data - jp->initial;
        return false;
    }
    return true;
}

bool jp_null(JsonParser *jp) {
    if (!jp__value_handle_state(jp, jp->source)) return false;

    if (!sv_strip_prefix(&jp->source, JSON__NULL)) {
        jp->error.count = 0;
        sb_append_sv(&jp->error, SV_FROM_LITERAL("expected null, received "));
        if (jp->source.count) {
            sb_appendf(&jp->error, "'%c'", sv_get(jp->source, 0));
        } else {
            sb_append_sv(&jp->error, SV_FROM_LITERAL("EOF"));
        }
        jp->error_char = jp->source.data - jp->initial;
        return false;
    }
    return true;
}

bool jp_array_begin(JsonParser *jp) {
    StringView mark = jp->source;
    if (!jp__value_handle_state(jp, mark)) return false;

    if (!jp__expect(jp, '[', mark)) return false;
    Json__StateStack_push(&jp->stack, JSON_STATE_ARRAY_BEGIN);
    return true;
}

bool jp_array_end(JsonParser *jp) {
    ASSERT(jp->stack.count > 0,
           "Cannot end an array without beginning it first;"
           "did you mean to call `jp_array_begin` instead?");

    Json__State state = Json__StateStack_pop(&jp->stack);
    switch (state) {
    case JSON_STATE_OBJECT_BEGIN: // fallthrough
    case JSON_STATE_OBJECT:       {
        ASSERT(false,
               "Cannot end an array when an object is open;"
               "did you mean to call `jp_object_end` instead?");
    } break;
    case JSON_STATE_KEY: {
        ASSERT(false,
               "Cannot end an array right after a key;"
               "did you mean to call `jp_array_begin` instead?");
    } break;
    case JSON_STATE_ARRAY_BEGIN: break;
    case JSON_STATE_ARRAY:       break;
    case JSON_STATE_STRING:      {
        UNREACHABLE("ಠ╭╮ಠ Have you been screwing with `jp->stack`?");
    } break;
    }

    StringView mark = jp->source;
    sv_trim_start(&jp->source);
    if (!jp__expect(jp, ']', mark)) return false;
    return true;
}

bool jp_object_begin(JsonParser *jp) {
    StringView mark = jp->source;
    if (!jp__value_handle_state(jp, mark)) return false;

    if (!jp__expect(jp, '{', mark)) return false;
    Json__StateStack_push(&jp->stack, JSON_STATE_OBJECT_BEGIN);
    return true;
}

bool jp_object_end(JsonParser *jp) {
    ASSERT(jp->stack.count > 0,
           "Cannot end an object without beginning it first;"
           "did you mean to call `jp_object_begin` instead?");

    Json__State state = Json__StateStack_pop(&jp->stack);
    switch (state) {
    case JSON_STATE_ARRAY_BEGIN: // fallthrough
    case JSON_STATE_ARRAY:       {
        ASSERT(false,
               "Cannot end an object when an array is open;"
               "did you mean to call `jp_array_end` instead?");
    } break;
    case JSON_STATE_KEY: {
        ASSERT(false,
               "Cannot end an object right after a key;"
               "did you mean to call `jp_object_begin` instead?");
    } break;
    case JSON_STATE_STRING: {
        UNREACHABLE("ಠ╭╮ಠ Have you been screwing with `jp->stack`?");
    } break;
    case JSON_STATE_OBJECT_BEGIN: break;
    case JSON_STATE_OBJECT:       break;
    }

    StringView mark = jp->source;
    sv_trim_start(&jp->source);
    if (!jp__expect(jp, '}', mark)) return false;
    return true;
}

bool jp_key(JsonParser *jp, StringBuilder *out) {
    out->count = 0;
    return jp_key_append(jp, out);
}

bool jp_key_append(JsonParser *jp, StringBuilder *out) {
    StringView mark = jp->source;
    if (!jp__key_handle_state(jp, mark)) return false;

    if (!jp__expect(jp, '"', mark)) return false;
    if (!jp__parse_string(jp, out, mark)) return false;
    if (!jp__expect(jp, '"', mark)) return false;
    sv_trim(&jp->source);
    if (!jp__expect(jp, ':', mark)) return false;

    Json__StateStack_push(&jp->stack, JSON_STATE_KEY);
    return true;
}

#define JSON__ERROR_PRINTF(jp, fn, arg)                                        \
    do {                                                                       \
        fn((arg), "JsonParser: at char " I32_FMT ": " SB_FMT "\n",             \
           (jp)->error_char, SB_ARG((jp)->error));                             \
    } while (0)

StringView jp_get_error(Arena *arena, JsonParser *jp) {
    StringBuilder sb = sb_new(arena, 152 + (jp->error_char / 10));
    JSON__ERROR_PRINTF(jp, sb_appendf, &sb);
    return sb_to_sv(sb);
}

void jp_print_error(JsonParser *jp, FILE *stream) {
    JSON__ERROR_PRINTF(jp, fprintf, stream);
}

#undef JSON__ERROR_PRINTF

struct JsonBuilder {
    StringBuilder sb;
    i32 level, padding;
    Json__StateStack stack;
};

void jb__string_render_char(JsonBuilder *jb, char c) {
    switch (c) {
    case '\n': sb_append_cstr(&jb->sb, "\\n"); break;
    case '\r': sb_append_cstr(&jb->sb, "\\r"); break;
    case '\t': sb_append_cstr(&jb->sb, "\\t"); break;
    case '\\': sb_append_cstr(&jb->sb, "\\\\"); break;
    case '"':  sb_append_cstr(&jb->sb, "\\\""); break;
    default:   sb_push(&jb->sb, c); break;
    }
}

void jb__render_padding(JsonBuilder *jb) {
    i32 padding = jb->padding;
    if (padding > 0) {
        sb_push(&jb->sb, '\n');
        i32 level = jb->level;
        for (i32 i = 0; i < level; i++) {
            for (i32 j = 0; j < padding; j++) sb_push(&jb->sb, ' ');
        }
    }
}

void jb__key_handle_state(JsonBuilder *jb) {
    ASSERT(jb->stack.count > 0,
           "Cannot render a key outside an object;"
           "did you forget to call `jb_object_begin`?");

    Json__State *state_ref = Json__StateStack_get_ref(jb->stack, -1);

    switch (*state_ref) {
    case JSON_STATE_OBJECT_BEGIN: {
        *state_ref = JSON_STATE_OBJECT;
    } break;
    case JSON_STATE_OBJECT: {
        sb_push(&jb->sb, ',');
    } break;
    case JSON_STATE_KEY: {
        ASSERT(false,
               "Cannot render a key twice in a row;"
               "did you forget to render a value after calling `jb_key`?");
    } break;
    case JSON_STATE_ARRAY_BEGIN: // fallthrough
    case JSON_STATE_ARRAY:       {
        ASSERT(false,
               "Cannot render a key into an array;"
               "did you forget to call `jb_object_begin`?");
    } break;
    case JSON_STATE_STRING: {
        ASSERT(false,
               "Unclosed string;"
               "did you forget to call `jb_string_end`?");
    } break;
    }

    jb__render_padding(jb);
    Json__StateStack_push(&jb->stack, JSON_STATE_KEY);
}

void jb__value_handle_state(JsonBuilder *jb) {
    if (!jb->stack.count) return;

    if (jb->stack.count < 0) {
        ASSERT(false,
               "Cannot render two values into the same builder."
               "You may want to reset the builder after using the result.");
    }

    Json__State *state_ref = Json__StateStack_get_ref(jb->stack, -1);

    switch (*state_ref) {
    case JSON_STATE_OBJECT_BEGIN: // fallthrough
    case JSON_STATE_OBJECT:       {
        ASSERT(false,
               "Cannot render a value directly into an object;"
               "did you forget to call `jb_key`?");
    } break;

    case JSON_STATE_KEY: {
        Json__StateStack_pop(&jb->stack);
    } break;
    case JSON_STATE_ARRAY_BEGIN: {
        *state_ref = JSON_STATE_ARRAY;
        jb__render_padding(jb);
    } break;
    case JSON_STATE_ARRAY: {
        sb_push(&jb->sb, ',');
        jb__render_padding(jb);
    } break;
    case JSON_STATE_STRING: {
        ASSERT(false,
               "Unclosed string;"
               "did you forget to call `jb_string_end`?");
    } break;
    }
}

JsonBuilder *jb_new_opt(Arena *arena, i32 capacity, JsonBuilderNewOpt opt) {
    JsonBuilder *jb = arena_alloc(arena, sizeof(JsonBuilder));
    jb->level = 0;
    jb->padding = opt.padding;
    if (!opt.max_depth) opt.max_depth = 64;
    jb->stack = Json__StateStack_new(arena, opt.max_depth);
    jb->sb = sb_new(arena, capacity);
    return jb;
}

void jb_begin(JsonBuilder *jb) {
    jb->level = 0;
    jb->stack.count = 0;
    jb->sb.count = 0;
}

void jb_integer(JsonBuilder *jb, i32 value) {
    jb__value_handle_state(jb);

    sb_appendf(&jb->sb, I32_FMT, value);
}

void jb_float(JsonBuilder *jb, float value) {
    jb__value_handle_state(jb);

    sb_appendf(&jb->sb, "%f", value);
}

void jb_string_view(JsonBuilder *jb, StringView value) {
    jb__value_handle_state(jb);

    sb_push(&jb->sb, '"');
    while (value.count) jb__string_render_char(jb, sv_shift(&value));
    sb_push(&jb->sb, '"');
}

void jb_string_begin(JsonBuilder *jb) {
    jb__value_handle_state(jb);

    sb_push(&jb->sb, '"');
    Json__StateStack_push(&jb->stack, JSON_STATE_STRING);
}

void jb_string_append(JsonBuilder *jb, StringView value) {
    ASSERT(jb->stack.count > 0 &&
               Json__StateStack_get(jb->stack, -1) == JSON_STATE_STRING,
           "Not in string mode;"
           "did you forget to call `jb_string_begin`?");
    while (value.count) jb__string_render_char(jb, sv_shift(&value));
}

void jb_string_push(JsonBuilder *jb, char c) {
    ASSERT(jb->stack.count > 0 &&
               Json__StateStack_get(jb->stack, -1) == JSON_STATE_STRING,
           "Not in string mode;"
           "did you forget to call `jb_string_begin`?");
    jb__string_render_char(jb, c);
}

void jb_string_end(JsonBuilder *jb) {
    ASSERT(jb->stack.count > 0 &&
               Json__StateStack_pop(&jb->stack) == JSON_STATE_STRING,
           "Not in string mode;"
           "did you forget to call `jb_string_begin`?");
    sb_push(&jb->sb, '"');
}

void jb_bool(JsonBuilder *jb, bool value) {
    jb__value_handle_state(jb);

    sb_append_cstr(&jb->sb, value ? "true" : "false");
}

void jb_null(JsonBuilder *jb) {
    jb__value_handle_state(jb);

    sb_append_cstr(&jb->sb, "null");
}

void jb_array_begin(JsonBuilder *jb) {
    jb__value_handle_state(jb);

    sb_push(&jb->sb, '[');
    jb->level++;
    Json__StateStack_push(&jb->stack, JSON_STATE_ARRAY_BEGIN);
}

void jb_array_end(JsonBuilder *jb) {
    ASSERT(jb->stack.count > 0,
           "Cannot end an array without beginning it first;"
           "did you mean to call `jb_array_begin` instead?");

    Json__State state = Json__StateStack_pop(&jb->stack);
    switch (state) {
    case JSON_STATE_OBJECT_BEGIN: // fallthrough
    case JSON_STATE_OBJECT:       {
        ASSERT(false,
               "Cannot end an array when an object is open;"
               "did you mean to call `jb_object_end` instead?");
    } break;
    case JSON_STATE_KEY: {
        ASSERT(false,
               "Cannot end an array right after a key;"
               "did you mean to call `jb_array_begin` instead?");
    } break;
    case JSON_STATE_STRING: {
        ASSERT(false,
               "Unclosed string;"
               "did you forget to call `jb_string_end`?");
    } break;
    case JSON_STATE_ARRAY_BEGIN: break;
    case JSON_STATE_ARRAY:       break;
    }

    jb->level--;
    jb__render_padding(jb);
    sb_push(&jb->sb, ']');
};

void jb_object_begin(JsonBuilder *jb) {
    jb__value_handle_state(jb);

    sb_push(&jb->sb, '{');
    jb->level++;
    Json__StateStack_push(&jb->stack, JSON_STATE_OBJECT_BEGIN);
}

void jb_object_end(JsonBuilder *jb) {
    ASSERT(jb->stack.count > 0,
           "Cannot end an object without beginning it first;"
           "did you mean to call `jb_object_begin` instead?");

    Json__State state = Json__StateStack_pop(&jb->stack);
    switch (state) {
    case JSON_STATE_ARRAY_BEGIN: // fallthrough
    case JSON_STATE_ARRAY:       {
        ASSERT(false,
               "Cannot end an object when an array is open;"
               "did you mean to call `jb_array_end` instead?");
    } break;
    case JSON_STATE_KEY: {
        ASSERT(false,
               "Cannot end an object into a key;"
               "did you mean to call `jb_object_begin` instead?");
    } break;
    case JSON_STATE_STRING: {
        ASSERT(false,
               "Unclosed string;"
               "did you forget to call `jb_string_end`?");
    } break;
    case JSON_STATE_OBJECT_BEGIN: break;
    case JSON_STATE_OBJECT:       break;
    }

    jb->level--;
    jb__render_padding(jb);
    sb_push(&jb->sb, '}');
}

void jb_key(JsonBuilder *jb, StringView key) {
    jb__key_handle_state(jb);

    sb_push(&jb->sb, '"');
    while (key.count) jb__string_render_char(jb, sv_shift(&key));
    sb_append_cstr(&jb->sb, "\":");
    if (jb->padding > 0) sb_push(&jb->sb, ' ');
}

void jb_key_cstr(JsonBuilder *jb, const char *key) {
    jb_key(jb, sv_from_cstr(key));
}

StringView jb_end(JsonBuilder *jb) {
    if (jb->stack.count > 0) {
        Json__State state = Json__StateStack_get(jb->stack, -1);
        switch (state) {
        case JSON_STATE_OBJECT_BEGIN: // fallthrough
        case JSON_STATE_OBJECT:       {
            ASSERT(false,
                   "Cannot call `jb_end` while an object is in progress;"
                   "did you forget to call `jb_object_end`?");
        } break;
        case JSON_STATE_KEY: {
            ASSERT(false,
                   "Cannot call `jb_end` while waiting for key value;"
                   "did you accidentally call `jb_key`?");
        } break;
        case JSON_STATE_ARRAY_BEGIN: // fallthrough
        case JSON_STATE_ARRAY:       {
            ASSERT(false,
                   "Cannot call `jb_end` while an array is in progress;"
                   "did you forget to call `jb_array_end`?");
        } break;
        case JSON_STATE_STRING: {
            ASSERT(false,
                   "Cannot call `jb_end` while a string is in progress;"
                   "did you forget to call `jb_string_end`?");

        } break;
        }
    }
    jb->stack.count = -1;

    return sb_to_sv(jb->sb);
}

#endif // BOOKSTORE_IMPLEMENTATION

#endif // JSON_H_
