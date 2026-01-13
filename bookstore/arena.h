/* arena.h */
/* Arena allocators and temporary lifetimes */

#ifndef ARENA_H_
#define ARENA_H_

#include "./basic.h"

// An arena allocator, which can be used to allocate and free memory in blocks.
typedef struct {
    // The amount of memory which can be allocated in this arena, in bytes.
    i32 capacity;
    // The amount of bytes which have currently been allocated with this arena.
    i32 allocated;
} Arena;

// Get the pointer to the arena's memory.
#define ARENA_MEMORY(arena) (i8 *)((arena) + 1)

// Create a new arena with `capacity` bytes, using `MALLOC`.
Arena *arena_new(i32 capacity);
// Destroy an arena, using `FREE`.
void arena_destroy(Arena *self);
// Allocate `size` bytes of memory using an arena allocator.
void *arena_alloc(Arena *self, i32 size);
// Clear an arena, freeing the capacity such that more memory can be allocated.
// Note: every `arena_alloc` onwards will overwrite previously allocated memory.
void arena_clear(Arena *self);
char *arena_clone_cstr(Arena *self, const char *cstr);
char *arena_sprintf(Arena *arena, const char *fmt, ...) PRINTF_FORMAT(2, 3);

// A temporary lifetime, associated with an arena allocator, which provides the
// ability to allocate memory for a temporary while and then reset the arena
// back to the state it was at the beginning of the lifetime.
typedef struct {
    Arena *arena;
    i32 start;
} Lifetime;

// Create a lifetime from an arena allocator.
Lifetime lifetime_begin(Arena *arena);
// End a lifetime, freeing the capacity of the associated arena back to the
// state it was in when the lifetime began.
void lifetime_end(Lifetime self);

#ifdef BOOKSTORE_IMPLEMENTATION

// TODO: null assertions

Arena *arena_new(i32 capacity) {
    Arena *self = (Arena *)MALLOC(sizeof(Arena) + capacity);
    ASSERT(self != NULL, "unable to allocate memory for arena");
    self->capacity = capacity;
    self->allocated = 0;
    return self;
}

void arena_destroy(Arena *self) {
    free(self);
}

void *arena_alloc(Arena *self, i32 size) {
    ASSERT(self->capacity >= self->allocated + size, "arena out of memory");

    void *data = ARENA_MEMORY(self) + self->allocated;
    self->allocated += size;
    return data;
}

void arena_clear(Arena *self) {
    self->allocated = 0;
}

char *arena_clone_cstr(Arena *self, const char *cstr) {
    i32 count = strlen(cstr) + 1;
    char *dest = arena_alloc(self, count * sizeof(char));
    MEMCPY(dest, cstr, count);
    return dest;
}

char *arena_sprintf(Arena *arena, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    i32 n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    char *dest = arena_alloc(arena, n + 1);
    va_start(args, fmt);
    vsnprintf(dest, n + 1, fmt, args);
    va_end(args);

    return dest;
}

Lifetime lifetime_begin(Arena *arena) {
    Lifetime self = {
        .arena = arena,
        .start = arena->allocated,
    };
    return self;
}

void lifetime_end(Lifetime self) {
    ASSERT(self.arena->capacity >= self.start && self.start >= 0,
           "invalid lifetime");

    self.arena->allocated = self.start;
}

#endif // ARENA_IMPLEMENTATION

#endif // ARENA_H_
