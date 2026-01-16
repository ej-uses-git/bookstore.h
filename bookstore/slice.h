/* slice.h */
/* Slices - sized pointers to memory */

#ifndef SLICE_H_
#define SLICE_H_

#include "./basic.h"
#include <stdbool.h>

// Define the necessary fields for a slice of type `T`.
// Should be placed in a `struct` definition.
#define SLICE_FIELDS(T)                                                        \
    /* The memory that the slice points to. */                                 \
    const T *data;                                                             \
    /* The amount of items in `data`. */                                       \
    i32 count
// A `typedef` for a struct with only the necessary fields for a slice of type
// `T`, with the name `name`.
#define SLICE_TYPEDEF(T, name)                                                 \
    typedef struct name {                                                      \
        SLICE_FIELDS(T);                                                       \
    } name
// Declare functions for a slice of type `T`, named `name`.
//
// Prefix all the functions with `name` by default - use `SLICE_DECLARE_PREFIX`
// to manually supply a prefix.
//
// Can be placed in a header file.
#define SLICE_DECLARE(T, name) SLICE_DECLARE_PREFIX(T, name, name)
// Declare functions for a slice of type `T`, named `name`.
//
// Prefix all the functions with `prefix`.
//
// Can be placed in a header file.
#define SLICE_DECLARE_PREFIX(T, name, prefix)                                  \
    /*                                                                         \
     * Create a slice that points to `count` elements of `parts`.              \
     */                                                                        \
    name prefix##_from_parts(const T *parts, i32 count);                       \
    /*                                                                         \
     * Copy a slice.                                                           \
     *                                                                         \
     * Does not copy the underlying data, only the pointer and the count.      \
     */                                                                        \
    name prefix##_copy(name self);                                             \
    /*                                                                         \
     * Get the item at index `i` from the slice `self`, as a value.            \
     *                                                                         \
     * Negative indexes are supported - a negative `i` is computed as          \
     * `self.count - i`, so `-1` can be used to refer to the last element in   \
     * the slice.                                                              \
     *                                                                         \
     * `ASSERT` that `i` is within the bounds of the slice.                    \
     */                                                                        \
    T prefix##_get(name self, i32 i);                                          \
    /*                                                                         \
     * Get the item at index `i` from the slice `self`, as a pointer.          \
     *                                                                         \
     * Negative indexes are supported - a negative `i` is computed as          \
     * `self.count - i`, so `-1` can be used to refer to the last element in   \
     * the slice.                                                              \
     *                                                                         \
     * `ASSERT` that `i` is within the bounds of the slice.                    \
     */                                                                        \
    const T *prefix##_get_ref(name self, i32 i);                               \
    /*                                                                         \
     * Find the index of item `item` in the slice `self`.                      \
     *                                                                         \
     * Returns a negative number in case of failure, which will not be         \
     * accepted by `slice_get` or `slice_get_ref`.                             \
     */                                                                        \
    i32 prefix##_index_of(name self, T item);                                  \
    /*                                                                         \
     * Remove the `count` elements at the start of the slice, returning those  \
     * elements as a new slice.                                                \
     *                                                                         \
     * Does not modify the underlying data being pointed to.                   \
     */                                                                        \
    name prefix##_strip_start(name *self, i32 count);                          \
    /*                                                                         \
     * Remove the `count` elements at the end of the slice, returning those    \
     * elements as a new slice.                                                \
     *                                                                         \
     * Does not modify the underlying data being pointed to.                   \
     */                                                                        \
    name prefix##_strip_end(name *self, i32 count);                            \
    /*                                                                         \
     * Split a slice into two by a given delimiter item, returning the         \
     * elements before the first instance of the delimiter and setting `self`  \
     * to point to the rest of the slice after that instance.                  \
     *                                                                         \
     * Neither slice includes the actual delimiter.                            \
     *                                                                         \
     * Does not modify the underlying data being pointed to.                   \
     */                                                                        \
    name prefix##_cut_delimiter(name *self, T delimiter);                      \
    /*                                                                         \
     * Split a slice into two by a given delimiter item, returning the         \
     * elements before the last instance of the delimiter and setting `self`   \
     * to point to the rest of the slice after that instance.                  \
     *                                                                         \
     * Neither slice includes the actual delimiter.                            \
     *                                                                         \
     * Does not modify the underlying data being pointed to.                   \
     */                                                                        \
    name prefix##_cut_delimiter_end(name *self, T delimiter);                  \
    /*                                                                         \
     * Remove the first element from a slice, returning it as a value.         \
     *                                                                         \
     * `ASSERT` that the slice isn't empty.                                    \
     *                                                                         \
     * Does not modify the underlying data being pointed to.                   \
     */                                                                        \
    T prefix##_shift(name *self);                                              \
    /*                                                                         \
     * Remove the last element from a slice, returning it as a value.          \
     *                                                                         \
     * `ASSERT` that the slice isn't empty.                                    \
     *                                                                         \
     * Does not modify the underlying data being pointed to.                   \
     */                                                                        \
    T prefix##_pop(name *self);                                                \
    /*                                                                         \
     * Check if two slices are equal to one another, by comparing their sizes  \
     * and looping through their elements in parallel.                         \
     */                                                                        \
    bool prefix##_eq(name self, name other);                                   \
    /*                                                                         \
     * Check if the slice `self` begins with the slice `other`.                \
     *                                                                         \
     * If `self` is smaller than `other`, returns `false`.                     \
     */                                                                        \
    bool prefix##_starts_with(name self, name other);                          \
    /*                                                                         \
     * Check if the slice `self` ends with the slice `other`.                  \
     *                                                                         \
     * If `self` is smaller than `other`, returns `false`.                     \
     */                                                                        \
    bool prefix##_ends_with(name self, name other);                            \
    /*                                                                         \
     * Check if the slice `self` begins with the slice `other`, and if so,     \
     * strips the start of `self` to remove the amount of elements in the      \
     * prefix.                                                                 \
     *                                                                         \
     * If `self` is smaller than `other`, returns `false`.                     \
     */                                                                        \
    bool prefix##_strip_prefix(name *self, name start);                        \
    /*                                                                         \
     * Check if the slice `self` ends with the slice `other`, and if so,       \
     * strips the end of `self` to remove the amount of elements in the        \
     * prefix.                                                                 \
     *                                                                         \
     * If `self` is smaller than `other`, returns `false`.                     \
     */                                                                        \
    bool prefix##_strip_suffix(name *self, name end)
// Define functions for a slice of type `T`, named `name`.
//
// Values of type `T` will be compared using `==` - use `SLICE_DEFINE_COMPLEX`
// to manually supply comparison logic.
//
// Prefix all the functions with `name` by default - use `SLICE_DEFINE_PREFIX`
// to manually supply a prefix.
//
// Companion to `SLICE_DECLARE`.
#define SLICE_DEFINE(T, name) SLICE_DEFINE_PREFIX(T, name, name)
// Define functions for a slice of type `T`, named `name`.
//
// Values of type `T` will be compared using `==` - use
// `SLICE_DEFINE_COMPLEX_PREFIX` to manually supply comparison logic.
//
// Prefix all the functions with `prefix`.
//
// Companion to `SLICE_DECLARE_PREFIX`.
#define SLICE_DEFINE_PREFIX(T, name, prefix)                                   \
    SLICE_DEFINE_COMPLEX_PREFIX(T, EQ, name, prefix)
// Define functions for a slice of type `T`, named `name`.
//
// Values of type `T` will be compared using `eq`, which should return a `bool`.
//
// Prefix all the functions with `name` by default - use
// `SLICE_DEFINE_COMPLEX_PREFIX` to manually supply a prefix.
//
// Companion to `SLICE_DECLARE`.
#define SLICE_DEFINE_COMPLEX(T, eq, name)                                      \
    SLICE_DEFINE_COMPLEX_PREFIX(T, name, name)
// Define functions for a slice of type `T`, named `name`.
//
// Values of type `T` will be compared using `eq`, which should return a `bool`.
//
// Prefix all the functions with `prefix`.
//
// Companion to `SLICE_DECLARE_PREFIX`.
#define SLICE_DEFINE_COMPLEX_PREFIX(T, eq, name, prefix)                       \
    name prefix##_from_parts(const T *parts, i32 count) {                      \
        name self = {.data = parts, .count = count};                           \
        return self;                                                           \
    }                                                                          \
    name prefix##_copy(name self) {                                            \
        return prefix##_from_parts(self.data, self.count);                     \
    }                                                                          \
    T prefix##_get(name self, i32 i) {                                         \
        if (i < 0) i = self.count + i;                                         \
        ASSERT(self.count > i && i >= 0, "index out of bounds");               \
        return self.data[i];                                                   \
    }                                                                          \
    const T *prefix##_get_ref(name self, i32 i) {                              \
        if (i < 0) i = self.count + i;                                         \
        ASSERT(self.count > i && i >= 0, "index out of bounds");               \
        return &self.data[i];                                                  \
    }                                                                          \
    i32 prefix##_index_of(name self, T item) {                                 \
        for (i32 i = 0; i < self.count; i++) {                                 \
            if (eq(self.data[i], item)) {                                      \
                return i;                                                      \
            }                                                                  \
        }                                                                      \
        return -self.count - 1;                                                \
    }                                                                          \
    name prefix##_strip_start(name *self, i32 count) {                         \
        if (count > self->count) count = self->count;                          \
        name stripped = prefix##_from_parts(self->data, count);                \
        self->data += count;                                                   \
        self->count -= count;                                                  \
        return stripped;                                                       \
    }                                                                          \
    name prefix##_strip_end(name *self, i32 count) {                           \
        if (count > self->count) count = self->count;                          \
        name stripped =                                                        \
            prefix##_from_parts(self->data + self->count - count, count);      \
        self->count -= count;                                                  \
        return stripped;                                                       \
    }                                                                          \
    T prefix##_shift(name *self) {                                             \
        ASSERT(self->count > 0, "cannot shift an empty slice");                \
        T ret = *self->data++;                                                 \
        self->count--;                                                         \
        return ret;                                                            \
    }                                                                          \
    T prefix##_pop(name *self) {                                               \
        ASSERT(self->count > 0, "cannot pop an empty slice");                  \
        return self->data[--self->count];                                      \
    }                                                                          \
    name prefix##_cut_delimiter(name *self, T delimiter) {                     \
        i32 i = 0;                                                             \
        while (i < self->count && !eq(self->data[i], delimiter)) {             \
            i += 1;                                                            \
        }                                                                      \
        name result = prefix##_from_parts(self->data, i);                      \
        if (i < self->count) {                                                 \
            self->count -= i + 1;                                              \
            self->data += i + 1;                                               \
        } else {                                                               \
            self->count = 0;                                                   \
        }                                                                      \
        return result;                                                         \
    }                                                                          \
    name prefix##_cut_delimiter_end(name *self, T delimiter) {                 \
        i32 i = self->count - 1;                                               \
        while (i >= 0 && !eq(self->data[i], delimiter)) {                      \
            i -= 1;                                                            \
        }                                                                      \
        name result;                                                           \
        if (i >= 0) {                                                          \
            result = prefix##_from_parts(self->data, i);                       \
            self->count -= i + 1;                                              \
            self->data += i + 1;                                               \
        } else {                                                               \
            result = prefix##_copy(*self);                                     \
            self->count = 0;                                                   \
        }                                                                      \
        return result;                                                         \
    }                                                                          \
    bool prefix##_eq(name self, name other) {                                  \
        if (self.count != other.count) return false;                           \
        for (i32 i = 0; i < self.count; i++) {                                 \
            if (!eq(prefix##_get(self, i), prefix##_get(other, i)))            \
                return false;                                                  \
        }                                                                      \
        return true;                                                           \
    }                                                                          \
    bool prefix##_starts_with(name self, name other) {                         \
        if (self.count < other.count) return false;                            \
        for (i32 i = 0; i < other.count; i++) {                                \
            if (!eq(prefix##_get(self, i), prefix##_get(other, i)))            \
                return false;                                                  \
        }                                                                      \
        return true;                                                           \
    }                                                                          \
    bool prefix##_ends_with(name self, name other) {                           \
        if (self.count < other.count) return false;                            \
        for (i32 i = 0; i < other.count; i++) {                                \
            i32 negative = -i - 1;                                             \
            if (!eq(prefix##_get(self, negative),                              \
                    prefix##_get(other, negative)))                            \
                return false;                                                  \
        }                                                                      \
        return true;                                                           \
    }                                                                          \
    bool prefix##_strip_prefix(name *self, name start) {                       \
        if (!prefix##_starts_with(*self, start)) return false;                 \
        prefix##_strip_start(self, start.count);                               \
        return true;                                                           \
    }                                                                          \
    bool prefix##_strip_suffix(name *self, name end) {                         \
        if (!prefix##_ends_with(*self, end)) return false;                     \
        prefix##_strip_end(self, end.count);                                   \
        return true;                                                           \
    }

#endif // SLICE_H_
