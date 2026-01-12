/* random.h */
/* Random number generation (PCG) */

/*
 * I have modified the files in the following ways:
 *
 * - Made code into a header-only library
 * - Used type definitions from basic.h
 * - Renamed things to fit a simpler naming convention
 * - Added documentation comments
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

// The internal state used by `Random`.
struct RandomState {
    u32 state;
    u32 inc;
};

// A random number generator.
typedef struct RandomState Random;

// Seed a `Random` instance.
// This must be done before numbers can be properly generated.
void random_seed_r(Random *rng, u64 initstate, u64 initseq);
// Get a random number from a `Random` instance, advancing the state.
u32 random_next_r(Random *rng);
// Get a random number from a `Random` instance, advancing the state.
// Limit the returned number to a maximum bound of `bound`.
u32 random_next_bounded_r(Random *rng, u32 bound);

// Seed the global `Random` instance.
// This must be done before numbers can be properly generated.
void random_seed(u64 initstate, u64 initseq);
// Get a random number from the global `Random` instance, advancing the state.
u32 random_next(void);
// Get a random number from the global `Random` instance, advancing the state.
// Limit the returned number to a maximum bound of `bound`.
u32 random_next_bounded(u32 bound);

#ifdef BOOKSTORE_IMPLEMENTATION

void random_seed_r(Random *rng, u64 initstate, u64 initseq) {
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    random_next_r(rng);
    rng->state += initstate;
    random_next_r(rng);
}

u32 random_next_r(Random *rng) {
    u64 oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    u32 xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    u32 rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

u32 random_next_bounded_r(Random *rng, u32 bound) {
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
        u32 r = random_next_r(rng);
        if (r >= threshold) return r % bound;
    }
}

global Random rng_global;

void random_seed(u64 initstate, u64 initseq) {
    random_seed_r(&rng_global, initstate, initseq);
}

u32 random_next(void) {
    return random_next_r(&rng_global);
}

u32 random_next_bounded(u32 bound) {
    return random_next_bounded_r(&rng_global, bound);
}

#endif // BOOKSTORE_IMPLEMENTATION

#endif // RANDOM_H_
