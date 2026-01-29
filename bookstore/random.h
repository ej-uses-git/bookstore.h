/* random.h */
/* Random number generation (PCG) */

/*
 * I have modified the files in the following ways:
 *
 * - Made code into a header-only library
 * - Used type definitions from basic.h
 * - Renamed things to fit a simpler naming convention
 * - Added documentation comments
 * - Added wrapper functions for all sorts of data types
 *
 * The license from pcg-random.org:
 *
 * PCG Random Number Generation for C.
 *
 * Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *     http://www.pcg-random.org
 */

#ifndef RANDOM_H_
#define RANDOM_H_

#include "./basic.h"
#include <limits.h>
#include <stdbool.h>

// The internal state used by `Random`.
struct RandomState {
    u64 state;
    u64 inc;
};

// A random number generator.
typedef struct RandomState Random;

Random rng_global;

// Seed a `Random` instance.
// This must be done before numbers can be properly generated.
void random_seed(Random *rng, u64 initstate, u64 initseq);
// Get a random `u32` from a `Random` instance, advancing the state.
u32 random_next_u32(Random *rng);
// Get a random `u32` from a `Random` instance, advancing the state.
// Limit the returned number to a maximum bound of `bound` (exclusive).
u32 random_next_u32_bounded(Random *rng, u32 bound);
// Get a random `bool` from a `Random` instance, advancing the state.
bool random_next_bool(Random *rng);
// Get a random `u8` from a `Random` instance, advancing the state.
u8 random_next_u8(Random *rng);
// Get a random `u8` from a `Random` instance, advancing the state.
// Limit the returned number to a maximum bound of `bound` (exclusive).
u8 random_next_u8_bounded(Random *rng, u8 bound);
// Get a random `u16` from a `Random` instance, advancing the state.
u16 random_next_u16(Random *rng);
// Get a random `u16` from a `Random` instance, advancing the state.
// Limit the returned number to a maximum bound of `bound` (exclusive).
u16 random_next_u16_bounded(Random *rng, u16 bound);
// Get a random `i8` from a `Random` instance, advancing the state.
i8 random_next_i8(Random *rng);
// Get a random `i8` from a `Random` instance, advancing the state.
// Limit the returned number to a range between `start` and `end` (inclusive).
i8 random_next_i8_ranged(Random *rng, i8 start, i8 end);
// Get a random `i16` from a `Random` instance, advancing the state.
i16 random_next_i16(Random *rng);
// Get a random `i16` from a `Random` instance, advancing the state.
// Limit the returned number to a range between `start` and `end` (inclusive).
i16 random_next_i16_ranged(Random *rng, i16 start, i16 end);
// Get a random `i32` from a `Random` instance, advancing the state.
i32 random_next_i32(Random *rng);
// Get a random `i32` from a `Random` instance, advancing the state.
// Limit the returned number to a range between `start` and `end` (inclusive).
i32 random_next_i32_ranged(Random *rng, i32 start, i32 end);
// Get a random `char` from a `Random` instance, advancing the state.
// Not limited to anything beyond `CHAR_MAX` and `CHAR_MIN`; see
// `random_next_char_ranged`, `RANDOM_NEXT_CHAR_MULTI_RANGED`,
// `random_next_ascii`, `random_next_alpha`, `random_next_numeric`, etc.
char random_next_char(Random *rng);
// Get a random `char` from a `Random` instance, advancing the state.
// Limit the returned character to a range between `start` and `end`
// (inclusive).
char random_next_char_ranged(Random *rng, char min, char max);
// Get a random `char` from a `Random` instance, advancing the state.
// Limit the returned character to one of the ranges in `range_table`, which
// should be a table of character pairs. `range_table_size` is the total amount
// of characters in the `range_table`, which must be even.
//
// For example:
//
// ```
// // Generate a character that's in either one of the ranges a-f and 0-9
// #define RANGE_COUNTS 2
// char range_table[RANGE_COUNTS*2] = {'a', 'f',
//                                     '0, '9'};
// char c = random_next_char_multi_ranged(&rng, range_table, RANGE_COUNTS*2);
// ```
char random_next_char_multi_ranged(Random *rng, char *range_table,
                                   i32 range_table_size);
// Get a random `char` from a `Random` instance, advancing the state.
// Limit the returned character to one of the provided range pairs, which must
// be specified as an even number of characters.
//
// For example:
//
// ```
// // Generate a character that's in either one of the ranges a-f and 0-9
// char c = RANDOM_NEXT_CHAR_MULTI_RANGED(&rng, 'a', 'f', '0', '9');
// ```
#define RANDOM_NEXT_CHAR_MULTI_RANGED(rng, ...)                                \
    random_next_char_multi_ranged(rng, (char[]){__VA_ARGS__},                  \
                                  sizeof((char[]){__VA_ARGS__}))
// Get a random `char` from a `Random` instance, advancing the state.
// Limit the returned character to the ASCII character range.
char random_next_ascii(Random *rng);
// Get a random `char` from a `Random` instance, advancing the state.
// Limit the returned character to be a printable ASCII character.
char random_next_print(Random *rng);
// Get a random `char` from a `Random` instance, advancing the state.
// Limit the returned character to be a lowercase alphabetical letter.
char random_next_lower(Random *rng);
// Get a random `char` from a `Random` instance, advancing the state.
// Limit the returned character to be an uppercase alphabetical letter.
char random_next_upper(Random *rng);
// Get a random `char` from a `Random` instance, advancing the state.
// Limit the returned character to be an alphabetical letter.
char random_next_alpha(Random *rng);
// Get a random `char` from a `Random` instance, advancing the state.
// Limit the returned character to be a digit.
char random_next_digit(Random *rng);
// Get a random `char` from a `Random` instance, advancing the state.
// Limit the returned character to be and alphabetical letter or a numeric
// digit.
char random_next_alnum(Random *rng);

#ifdef BOOKSTORE_IMPLEMENTATION

void random_seed(Random *rng, u64 initstate, u64 initseq) {
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    random_next_u32(rng);
    rng->state += initstate;
    random_next_u32(rng);
}

u32 random_next_u32(Random *rng) {
    u64 oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    u32 xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    u32 rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

u32 random_next_u32_bounded(Random *rng, u32 bound) {
    // To avoid bias, we need to make the range of the RNG a multiple of
    // bound, which we do by dropping output less than a threshold.
    // A naive scheme to calculate the threshold would be to do
    //
    // ```
    // u32 threshold = 0x100000000ull % bound;
    // ```
    //
    // ...but 64-bit div/mod is slower than 32-bit div/mod (especially on
    // 32-bit platforms).  In essence, we do
    //
    // ```
    // u32 threshold = (0x100000000ull-bound) % bound;
    // ```
    //
    // ...because this version will calculate the same modulus, but the
    // left-hand side value is less than 2^32.

    u32 threshold = -bound % bound;

    // Uniformity guarantees that this loop will terminate.  In practice, it
    // should usually terminate quickly; on average (assuming all bounds are
    // equally likely), 82.25% of the time, we can expect it to require just
    // one iteration.  In the worst case, someone passes a bound of 2^31 + 1
    // (i.e., 2147483649), which invalidates almost 50% of the range.  In
    // practice, bounds are typically small and only a tiny amount of the range
    // is eliminated.
    for (;;) {
        u32 r = random_next_u32(rng);
        if (r >= threshold) return r % bound;
    }
}

bool random_next_bool(Random *rng) {
    return (bool)random_next_u32_bounded(rng, 2);
}

#define RANDOM__DEFINE_UNSIGNED(T, max)                                        \
    T random_next_##T(Random *rng) {                                           \
        return (T)random_next_u32_bounded(rng, (u32)max + 1);                  \
    }                                                                          \
    T random_next_##T##_bounded(Random *rng, T bound) {                        \
        return (T)random_next_u32_bounded(rng, bound);                         \
    }
#define RANDOM__DEFINE_SIGNED(T, max)                                          \
    T random_next_##T(Random *rng) {                                           \
        T result = (T)random_next_u32_bounded(rng, (u32)max + 1);              \
        bool negative = random_next_bool(rng);                                 \
        return negative ? -result : result;                                    \
    }                                                                          \
    T random_next_##T##_ranged(Random *rng, T start, T end) {                  \
        ASSERT(end >= start, "invalid range");                                 \
        T result = (T)random_next_u32_bounded(rng, (u32)(end - start + 1));    \
        return result + start;                                                 \
    }

RANDOM__DEFINE_UNSIGNED(u8, UINT8_MAX)
RANDOM__DEFINE_UNSIGNED(u16, UINT16_MAX)
RANDOM__DEFINE_SIGNED(i8, INT8_MAX)
RANDOM__DEFINE_SIGNED(i16, INT16_MAX)
RANDOM__DEFINE_SIGNED(i32, INT32_MAX)

char random_next_char(Random *rng) {
#if CHAR_MAX == INT8_MAX && CHAR_MIN == INT8_MIN
    return (char)random_next_i8(rng);
#else
    return (char)random_next_i16_ranged(rng, CHAR_MIN, CHAR_MAX);
#endif
}

char random_next_char_ranged(Random *rng, char min, char max) {
#if CHAR_MAX == INT8_MAX && CHAR_MIN == INT8_MIN
    return (char)random_next_i8_ranged(rng, min, max);
#else
    return (char)random_next_i16_ranged(rng, min, max);
#endif
}

char random_next_char_multi_ranged(Random *rng, char *range_table,
                                   i32 range_table_size) {
    ASSERT(range_table_size % 2 == 0,
           "invalid range table; the table must be made up of character pairs");
    i32 range_count = range_table_size / 2;
    i32 total = 0;
    for (i32 i = 0; i < range_count; i++) {
        char first = range_table[i * 2];
        char second = range_table[i * 2 + 1];
        i32 diff = second - first + 1;
        ASSERT(diff > 0, "invalid character range");
        total += diff;
    }

    u32 n = random_next_u32_bounded(rng, total);

    for (i32 i = 0; i < range_count; i++) {
        char first = range_table[i * 2];
        char second = range_table[i * 2 + 1];
        u32 diff = second - first + 1;
        if (n < diff) return n + first;
        n -= diff;
    }

    // NOTE: we know this is unreachable because `n` must be in the range,
    // based on the `total` calculation above
    UNREACHABLE("random_next_char_multi_ranged");
}

char random_next_ascii(Random *rng) {
    return random_next_char_ranged(rng, 0, 0177);
}

char random_next_print(Random *rng) {
    return random_next_char_ranged(rng, 040, 0176);
}

char random_next_lower(Random *rng) {
    return random_next_char_ranged(rng, 'a', 'z');
}

char random_next_upper(Random *rng) {
    return random_next_char_ranged(rng, 'A', 'Z');
}

char random_next_alpha(Random *rng) {
    return RANDOM_NEXT_CHAR_MULTI_RANGED(rng, 'a', 'z', 'A', 'Z');
}

char random_next_digit(Random *rng) {
    return random_next_char_ranged(rng, '0', '9');
}

char random_next_alnum(Random *rng) {
    return RANDOM_NEXT_CHAR_MULTI_RANGED(rng, 'a', 'z', 'A', 'Z', '0', '9');
}

#endif // BOOKSTORE_IMPLEMENTATION

#endif // RANDOM_H_
