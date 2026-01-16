/* array.h */
/* Dynamic arrays */

#ifndef ARRAY_H_
#define ARRAY_H_

#include "./arena.h"

// Define the necessary fields for a dynamic array of type `T`.
// Should be placed in a `struct` definition.
#define ARRAY_FIELDS(T)                                                        \
    /* The memory for the items of the array. */                               \
    T *items;                                                                  \
    /* The count of valid elements currently in the array. */                  \
    i32 count;                                                                 \
    /* The maximum amount of items that can be stored in the array. */         \
    i32 capacity
// A `typedef` for a struct with only the necessary fields for a dynamic array
// of type `T`, with the name `name`.
#define ARRAY_TYPEDEF(T, name)                                                 \
    typedef struct name {                                                      \
        ARRAY_FIELDS(T);                                                       \
    } name
// Declare functions for a dynamic array of type `T`, named `name`.
//
// Prefix all the functions with `name` by default - use `ARRAY_DECLARE_PREFIX`
// to manually supply a prefix.
//
// Can be placed in a header file.
#define ARRAY_DECLARE(T, name) ARRAY_DECLARE_PREFIX(T, name, name)
// Declare functions for a dynamic array of type `T`, named `name`.
//
// Prefix all the functions with `prefix`.
//
// Can be placed in a header file.
#define ARRAY_DECLARE_PREFIX(T, name, prefix)                                  \
    /*                                                                         \
     * Create a new array with capacity `capacity`.                            \
     *                                                                         \
     * If `arena` is not `NULL`, it is used to allocate the array's memory.    \
     *                                                                         \
     * Otherwise, the array is dynamically allocated using `MALLOC`.           \
     */                                                                        \
    name prefix##_new(Arena *arena, i32 capacity);                             \
    /*                                                                         \
     * Ensure the array `self` has at least `amount` memory available.         \
     *                                                                         \
     * If the array was dynamically allocated (passing `NULL` to `array_new`), \
     * the array can be resized.                                               \
     *                                                                         \
     * Otherwise, simply `ASSERT` that the `amount` is beneath the capacity.   \
     */                                                                        \
    void prefix##_reserve(name *self, i32 amount);                             \
    /*                                                                         \
     * Push `item` into the array `self`.                                      \
     *                                                                         \
     * Uses `array_reserve` behind the scenes to ensure that the array has     \
     * enough room for the new item.                                           \
     */                                                                        \
    void prefix##_push(name *self, T item);                                    \
    /*                                                                         \
     * Append `amount` items from `items` into the array `self`.               \
     * Uses `MEMCPY` to copy the memory into the array.                        \
     *                                                                         \
     * Uses `array_reserve` behind the scenes to ensure that the array has     \
     * enough room for the new items.                                          \
     */                                                                        \
    void prefix##_append(name *self, const T *items, i32 amount);              \
    /*                                                                         \
     * Append the array `other` into the array `self`.                         \
     * Uses `MEMCPY` to copy the memory into the array.                        \
     *                                                                         \
     * Uses `array_reserve` behind the scenes to ensure that the array has     \
     * enough room for the new items.                                          \
     */                                                                        \
    void prefix##_append_other(name *self, name other);                        \
    /*                                                                         \
     * Get the item at index `i` from the array `self`, as a value.            \
     *                                                                         \
     * Negative indexes are supported - a negative `i` is computed as          \
     * `self.count - i`, so `-1` can be used to refer to the last element in   \
     * the array.                                                              \
     *                                                                         \
     * `ASSERT` that `i` is within the bounds of the array.                    \
     */                                                                        \
    T prefix##_get(name self, i32 i);                                          \
    /*                                                                         \
     * Get the item at index `i` from the array `self`, as a pointer.          \
     *                                                                         \
     * Negative indexes are supported - a negative `i` is computed as          \
     * `self.count - i`, so `-1` can be used to refer to the last element in   \
     * the array.                                                              \
     *                                                                         \
     * `ASSERT` that `i` is within the bounds of the array.                    \
     */                                                                        \
    T *prefix##_get_ref(name self, i32 i);                                     \
    /*                                                                         \
     * Remove the last item from the array `self`, returning it as a value.    \
     *                                                                         \
     * `ASSERT` that the array isn't empty.                                    \
     */                                                                        \
    T prefix##_pop(name *self);                                                \
    /*                                                                         \
     * Remove the item at index `i` in the array `self`, returning it as a     \
     * value.                                                                  \
     *                                                                         \
     * Negative indexes are supported - a negative `i` is computed as          \
     * `self.count - i`.                                                       \
     *                                                                         \
     * This is done using the `swapback` algorithm, so the element is simply   \
     * replaced by the last element in the array, ruining the order of the     \
     * array. If the array is supposed to be ordered, you should not use this  \
     * function.                                                               \
     *                                                                         \
     * `ASSERT` that `i` is within the bounds of the array.                    \
     */                                                                        \
    T prefix##_remove_swapback(name *self, i32 i)
// Define functions for a dynamic array of type `T`, named `name`.
//
// Prefix all the functions with `name` by default - use `ARRAY_DEFINE_PREFIX`
// to manually supply a prefix.
//
// Companion to `ARRAY_DECLARE`.
#define ARRAY_DEFINE(T, name) ARRAY_DEFINE_PREFIX(T, name, name)
// Define functions for a dynamic array of type `T`, named `name`.
//
// Prefix all the functions with `prefix` by default.
//
// Companion to `ARRAY_DECLARE_PREFIX`.
#define ARRAY_DEFINE_PREFIX(T, name, prefix)                                   \
    name prefix##_new(Arena *arena, i32 capacity) {                            \
        name array = {0};                                                      \
        if (arena == NULL) {                                                   \
            if (capacity <= 0) {                                               \
                array.capacity = -256;                                         \
            } else {                                                           \
                array.capacity = capacity * -1;                                \
            }                                                                  \
            array.items =                                                      \
                (T *)MALLOC(array.capacity * -1 * sizeof(*array.items));       \
        } else {                                                               \
            array.capacity = capacity;                                         \
            array.items =                                                      \
                (T *)arena_alloc(arena, capacity * sizeof(*array.items));      \
        }                                                                      \
        array.count = 0;                                                       \
        return array;                                                          \
    }                                                                          \
    void prefix##_reserve(name *self, i32 amount) {                            \
        if (self->capacity < 0) {                                              \
            i32 capacity = self->capacity * -1;                                \
            while (capacity < self->count + amount) {                          \
                capacity *= 2;                                                 \
            }                                                                  \
            self->capacity = capacity * -1;                                    \
            self->items =                                                      \
                (T *)REALLOC(self->items, capacity * sizeof(*self->items));    \
        } else {                                                               \
            ASSERT(self->capacity >= amount,                                   \
                   MACRO_STRING(name) " at full capacity");                    \
        }                                                                      \
    }                                                                          \
    void prefix##_push(name *self, T item) {                                   \
        prefix##_reserve(self, self->count + 1);                               \
        self->items[self->count++] = item;                                     \
    }                                                                          \
    void prefix##_append(name *self, const T *items, i32 amount) {             \
        prefix##_reserve(self, self->count + amount);                          \
        T *dest = self->items + self->count;                                   \
        MEMCPY(dest, items, amount * sizeof(*items));                          \
        self->count += amount;                                                 \
    }                                                                          \
    void prefix##_append_other(name *self, name other) {                       \
        prefix##_append(self, other.items, other.count);                       \
    }                                                                          \
    T prefix##_get(name self, i32 i) {                                         \
        if (i < 0) i = self.count + i;                                         \
        ASSERT(self.count > i && i >= 0, "index out of bounds");               \
        return self.items[i];                                                  \
    }                                                                          \
    T *prefix##_get_ref(name self, i32 i) {                                    \
        if (i < 0) i = self.count + i;                                         \
        ASSERT(self.count > i && i >= 0, "index out of bounds");               \
        return &self.items[i];                                                 \
    }                                                                          \
    T prefix##_pop(name *self) {                                               \
        ASSERT(self->count > 0, "cannot pop empty array");                     \
        return self->items[--self->count];                                     \
    }                                                                          \
    T prefix##_remove_swapback(name *self, i32 i) {                            \
        if (i < 0) i = self->count + i;                                        \
        ASSERT(self->count > i && i >= 0, "index out of bounds");              \
        T item = self->items[i];                                               \
        self->items[i] = prefix##_pop(self);                                   \
        return item;                                                           \
    }

#endif // ARRAY_H_
