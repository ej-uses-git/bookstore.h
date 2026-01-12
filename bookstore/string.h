/* string.h */
/* String utilities - builders and views */

#ifndef STRING_H_
#define STRING_H_

#include "./array.h"
#include "./slice.h"
#include "arena.h"
#include <float.h>

// A sized string that doesn't own memory, so its size cannot be increased.
SLICE_TYPEDEF(char, StringView);
SLICE_DECLARE_PREFIX(char, StringView, sv);
// Used with `SV_ARG` to use a `StringView` in `printf`-style formatting.
//
// Example:
//
// ```
// printf("String: " SV_FMT, SV_ARG(sv));
// ```
#define SV_FMT "%.*s"
// Used with `SV_FMT` to use a `StringView` in `printf`-style formatting.
//
// Example:
//
// ```
// printf("String: " SV_FMT, SV_ARG(sv));
// ```
#define SV_ARG(sv) (int)(sv).count, (sv).data
// An empty `StringView`.
#define SV_EMPTY   (StringView){0}
#define SV_INVALID (StringView){.data = NULL, .count = -1}
// Create a `StringView` from a C-string (NUL-terminated list of characters).
StringView sv_from_cstr(const char *cstr);
// Copy over a `StringView` to a C-string (NUL-terminated list of characters),
// using `arena` to allocate the memory for the string.
const char *sv_to_cstr(Arena *arena, StringView self);
// Format `fmt` like `printf`, returning a `StringView` with the result.
//
// Uses `arena` to allocate the memory for the string - when `arena_clear` is
// called on it, the string also becomes invalidated.
StringView sv_printf(Arena *arena, const char *fmt, ...) PRINTF_FORMAT(2, 3);
// Check if a `StringView` is equal to a C-string (NUL-terminated list of
// characters).
bool sv_eq_cstr(StringView self, const char *cstr);
// Strip a `StringView` of whitespace characters.
void sv_trim(StringView *self);
// Strip the start of a `StringView` of whitespace characters.
void sv_trim_start(StringView *self);
// Strip the end of a `StringView` of whitespace characters.
void sv_trim_end(StringView *self);

#define STRING__SV_PARSE_INT_DECLARE(T)   T sv_parse_##T(StringView *self, T base)
#define STRING__SV_PARSE_FLOAT_DECLARE(T) T sv_parse_##T(StringView *self)
// Parse a `u32` from `self`, using a given base, consuming `self` so it points
// after where the numeric string ends. Uses `sv_trim_start` before parsing.
//
// If the number would overflow, `UINT32_MAX` is returned.
//
// If you expect the `StringView` to point to a string that is entirely a
// number, you can confirm the validity by checking if `self->count == 0` after
// the call.
//
// If you expect the `StringView` to have at least one numeric character, you
// can confirm the validity by storing `self->count` in a variable before the
// call, and check if `self->count > count` after.
STRING__SV_PARSE_INT_DECLARE(u32);
// Parse a `u64` from `self`, using a given base, consuming `self` so it points
// after where the numeric string ends. Uses `sv_trim_start` before parsing.
//
// If the number would overflow, `UINT64_MAX` is returned.
//
// If you expect the `StringView` to point to a string that is entirely a
// number, you can confirm the validity by checking if `self->count == 0` after
// the call.
//
// If you expect the `StringView` to have at least one numeric character, you
// can confirm the validity by storing `self->count` in a variable before the
// call, and check if `self->count > count` after.
STRING__SV_PARSE_INT_DECLARE(u64);
// Parse an `i32` from `self`, using a given base, consuming `self` so it points
// after where the numeric string ends. Uses `sv_trim_start` before parsing.
//
// If the number would overflow, `INT32_MAX` is returned.
//
// If you expect the `StringView` to point to a string that is entirely a
// number, you can confirm the validity by checking if `self->count == 0` after
// the call.
//
// If you expect the `StringView` to have at least one numeric character, you
// can confirm the validity by storing `self->count` in a variable before the
// call, and check if `self->count > count` after.
STRING__SV_PARSE_INT_DECLARE(i32);
// Parse an `i64` from `self`, using a given base, consuming `self` so it points
// after where the numeric string ends. Uses `sv_trim_start` before parsing.
//
// If the number would overflow, `INT64_MAX` is returned.
//
// If you expect the `StringView` to point to a string that is entirely a
// number, you can confirm the validity by checking if `self->count == 0` after
// the call.
//
// If you expect the `StringView` to have at least one numeric character, you
// can confirm the validity by storing `self->count` in a variable before the
// call, and check if `self->count > count` after.
STRING__SV_PARSE_INT_DECLARE(i64);
// Parse a `float` from `self`, consuming `self` so it points after where the
// numeric string ends. Uses `sv_trim_start` before parsing.
//
// If the number would overflow, `errno` is set to `ERANGE`.
//
// If you expect the `StringView` to point to a string that is entirely a
// number, you can confirm the validity by checking if `self->count == 0` after
// the call.
//
// If you expect the `StringView` to have at least one numeric character, you
// can confirm the validity by storing `self->count` in a variable before the
// call, and check if `self->count > count` after.
STRING__SV_PARSE_FLOAT_DECLARE(float);
// Parse a `double` from `self`, consuming `self` so it points after where the
// numeric string ends. Uses `sv_trim_start` before parsing.
//
// If the number would overflow, `errno` is set to `ERANGE`.
//
// If you expect the `StringView` to point to a string that is entirely a
// number, you can confirm the validity by checking if `self->count == 0` after
// the call.
//
// If you expect the `StringView` to have at least one numeric character, you
// can confirm the validity by storing `self->count` in a variable before the
// call, and check if `self->count > count` after.
STRING__SV_PARSE_FLOAT_DECLARE(double);

// A sized string that owns its memory and can be increased in size.
ARRAY_TYPEDEF(char, StringBuilder);
ARRAY_DECLARE_PREFIX(char, StringBuilder, sb);
// Used with `SB_ARG` to use a `StringBuilder` in `printf`-style formatting
//
// Example:
//
// ```
// printf("String: " SB_FMT, SB_ARG(sb));
// ```
#define SB_FMT "%.*s"
// Used with `SB_FMT` to use a `StringBuilder` in `printf`-style formatting
//
// Example:
//
// ```
// printf("String: " SB_FMT, SB_ARG(sb));
// ```
#define SB_ARG(sb) (int)(sb).count, (sb).items
// Format `fmt` like `printf`, appending the result to `self` and returning the
// amount of characters appended.
i32 sb_appendf(StringBuilder *self, const char *fmt, ...) PRINTF_FORMAT(2, 3);
// Push the NUL-terminating character (`'\0'`) to the end of `self`, allowing
// its underlying `items` to be used as a C-string.
void sb_push_null(StringBuilder *self);
// Append a C-string (NUL-terminated list of characters) into `self`.
// Uses `MEMCPY` to copy the memory into `self`.
void sb_append_cstr(StringBuilder *self, const char *cstr);
// Append a `StringView` into `self`.
// Uses `MEMCPY` to copy the memory into `self`.
void sb_append_sv(StringBuilder *self, StringView sv);
// Convert a `StringBuilder` into a `StringView`.
StringView sb_to_sv(StringBuilder self);

#ifdef BOOKSTORE_IMPLEMENTATION

#include <ctype.h>
#include <stdarg.h>

SLICE_DEFINE_PREFIX(char, StringView, sv)

StringView sv_from_cstr(const char *cstr) {
    return sv_from_parts(cstr, strlen(cstr));
}

const char *sv_to_cstr(Arena *arena, StringView self) {
    StringBuilder sb = sb_new(arena, self.count + 1);
    sb_append_sv(&sb, self);
    sb_push_null(&sb);
    return sb.items;
}

StringView sv_printf(Arena *arena, const char *fmt, ...) {
    if (!arena) TODO("dynamic sv_printf?");

    va_list args;

    va_start(args, fmt);
    i32 n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    char *dest = arena_alloc(arena, n * sizeof(char));
    va_start(args, fmt);
    vsnprintf(dest, n + 1, fmt, args);
    va_end(args);

    return sv_from_parts(dest, n);
}

bool sv_eq_cstr(StringView self, const char *cstr) {
    return sv_eq(self, sv_from_cstr(cstr));
}

void sv_trim(StringView *self) {
    sv_trim_start(self);
    sv_trim_end(self);
}

void sv_trim_start(StringView *self) {
    while (isspace(sv_get(*self, 0))) {
        sv_shift(self);
    }
}

void sv_trim_end(StringView *self) {
    while (isspace(sv_get(*self, -1))) {
        sv_pop(self);
    }
}

#define STRING__SV_PARSE_UINT_DEFINE(T, max)                                   \
    T sv_parse_##T(StringView *sv, T base) {                                   \
        sv_trim_start(sv);                                                     \
        if (!sv->count) {                                                      \
            sv->count = -1;                                                    \
            return 0;                                                          \
        }                                                                      \
        T cutoff = (T)(max) / base;                                            \
        T cutoff_mod = (T)(max) % base;                                        \
        T acc = 0;                                                             \
        while (sv->count) {                                                    \
            char c = sv_get(*sv, 0);                                           \
            T value;                                                           \
            if (isdigit(c)) {                                                  \
                value = c - '0';                                               \
            } else if (isalpha(c)) {                                           \
                value = c - isupper(c) ? 'A' : 'a';                            \
            } else {                                                           \
                break;                                                         \
            }                                                                  \
            if (value >= base) break;                                          \
            if (acc > cutoff || (acc == cutoff && value > cutoff_mod)) {       \
                acc = max;                                                     \
                break;                                                         \
            }                                                                  \
            sv_shift(sv);                                                      \
            acc *= base;                                                       \
            acc += value;                                                      \
        }                                                                      \
        return acc;                                                            \
    }
#define STRING__SV_PARSE_INT_DEFINE(T, max)                                    \
    T sv_parse_##T(StringView *sv, T base) {                                   \
        sv_trim_start(sv);                                                     \
        if (!sv->count) {                                                      \
            sv->count = -1;                                                    \
            return 0;                                                          \
        }                                                                      \
        char c = sv_get(*sv, 0);                                               \
        bool negative = false;                                                 \
        if (c == '-') {                                                        \
            sv_shift(sv);                                                      \
            negative = true;                                                   \
        } else if (c == '+') {                                                 \
            sv_shift(sv);                                                      \
        }                                                                      \
        T cutoff = (T)(max) / base;                                            \
        T cutoff_mod = (T)(max) % base;                                        \
        T acc = 0;                                                             \
        while (sv->count) {                                                    \
            char c = sv_shift(sv);                                             \
            T value;                                                           \
            if (isdigit(c)) {                                                  \
                value = c - '0';                                               \
            } else if (isalpha(c)) {                                           \
                value = c - isupper(c) ? 'A' : 'a';                            \
            } else {                                                           \
                break;                                                         \
            }                                                                  \
            if (value >= base) break;                                          \
            if (acc > cutoff || (acc == cutoff && value > cutoff_mod)) {       \
                acc = max;                                                     \
                break;                                                         \
            }                                                                  \
            acc *= base;                                                       \
            acc += value;                                                      \
        }                                                                      \
        if (negative) acc *= -1;                                               \
        return acc;                                                            \
    }
#define STRING__SV_PARSE_FLOAT_DEFINE(T, fn)                                   \
    T sv_parse_##T(StringView *sv) {                                           \
        char *endptr;                                                          \
        T ret = fn(sv->data, &endptr);                                         \
        i32 count = endptr - sv->data;                                         \
        sv->count -= count;                                                    \
        sv->data += count;                                                     \
        return ret;                                                            \
    }

STRING__SV_PARSE_UINT_DEFINE(u32, UINT32_MAX)
STRING__SV_PARSE_UINT_DEFINE(u64, UINT64_MAX)
STRING__SV_PARSE_INT_DEFINE(i32, INT32_MAX)
STRING__SV_PARSE_INT_DEFINE(i64, INT64_MAX)
STRING__SV_PARSE_FLOAT_DEFINE(float, strtof)
STRING__SV_PARSE_FLOAT_DEFINE(double, strtod)

ARRAY_DEFINE_PREFIX(char, StringBuilder, sb)

i32 sb_appendf(StringBuilder *self, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    i32 n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    // NOTE: the new_capacity needs to be +1 because of the null terminator.
    // However, further below we increase self->count by n, not n + 1.
    // This is because we don't want the `StringBuilder` to include the null
    // terminator. The user can always use `sb_push_null` if they want it.
    sb_reserve(self, n);
    char *dest = self->items + self->count;
    va_start(args, fmt);
    vsnprintf(dest, n + 1, fmt, args);
    va_end(args);

    self->count += n;

    return n;
}

void sb_append_cstr(StringBuilder *self, const char *cstr) {
    sb_append(self, cstr, strlen(cstr));
}

void sb_append_sv(StringBuilder *self, StringView sv) {
    sb_append(self, sv.data, sv.count);
}

void sb_push_null(StringBuilder *self) {
    sb_push(self, '\0');
}

StringView sb_to_sv(StringBuilder self) {
    return sv_from_parts(self.items, self.count);
}

#endif // BOOKSTORE_IMPLEMENTATION

#endif // STRING_H_
