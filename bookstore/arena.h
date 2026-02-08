/* arena.h */
/* Arena allocators and temporary lifetimes */

#ifndef ARENA_H_
#define ARENA_H_

#include "./basic.h"

// @type Arena
//
// @description
// An arena allocator, which can be used to allocate and free memory in blocks.
typedef struct {
    // The amount of memory which can be allocated in this arena, in bytes.
    i32 capacity;
    // The amount of bytes which have currently been allocated with this arena.
    i32 allocated;
} Arena;

// @macro_function ARENA_MEMORY
//
// @argument arena
// @returns The pointer to the arena's memory
#define ARENA_MEMORY(arena) (i8 *)((arena) + 1)

// @function arena_new
//
// @argument capacity The amount of bytes which can be allocated from the arena.
// @returns A pointer to the arena allocator.
//
// @description
// Create a new arena with `capacity` bytes, using `MALLOC`.
Arena *arena_new(i32 capacity);
// @function arena_destroy
//
// @argument arena The arena to destroy
//
// @description
// Destroy an arena, using `FREE`.
void arena_destroy(Arena *self);
// @function arena_alloc
//
// @argument self The arena to allocate the memory from
// @argument size The amount of bytes to allocate
// @returns A pointer to the allocated memory
//
// @description
// Allocate `size` bytes of memory using an arena allocator.
void *arena_alloc(Arena *self, i32 size);
// @function arena_clear
//
// @argument self The arena to clear.
//
// @description
// Clear an arena, freeing the capacity such that more memory can be allocated.
// Note: every `arena_alloc` onwards will overwrite previously allocated memory.
void arena_clear(Arena *self);
// @function arena_clone_cstr
//
// @argument arena The arena to allocate the memory for the clone from
// @argument cstr The C-string to clone
//
// @description
// Clone a C-string (NUL-terminated list of characters), using the arena to \
// allocate the underlying memory.
char *arena_clone_cstr(Arena *self, const char *cstr);
// @function arena_sprintf
//
// @argument self The arena to allocate the string's memory from
// @argument fmt The `printf` format string to use
// @argument ...rest The `printf` formatting arguments to use
// @returns A pointer to a C-string with the formatted data
//
// @description
// Format `fmt` like `printf`, returning a C-string (NUL-terminated list of \
// characters) with the result. Uses the arena to allocate the underlying \
// memory.
char *arena_sprintf(Arena *self, const char *fmt, ...) PRINTF_FORMAT(2, 3);

// @type Lifetime
//
// @description
// A temporary lifetime, associated with an arena allocator, which provides \
// the ability to allocate memory for a temporary while and then reset the \
// arena back to the state it was at the beginning of the lifetime.
typedef struct {
    // The arena allocator associated with this lifetime.
    Arena *arena;
    // The position in the arena where this lifetime begins, and to which the
    // arena should be reset once the lifetime ends.
    i32 start;
} Lifetime;

// @function lifetime_begin
//
// @argument arena The arena to create a lifetime from
// @returns A `Lifetime` for temporary allocations.
//
// @description
// Create a lifetime from an arena allocator.
//
// @example
// Lifetime lt = lifetime_begin(arena);
// (void)arena_alloc(lt.arena, MiB(1));
// lifetime_end(lt);
Lifetime lifetime_begin(Arena *arena);
// @function lifetime_end
//
// @argument self The `Lifetime` to end
//
// @description
// End a lifetime, freeing the capacity of the associated arena back to the \
// state it was in when the lifetime began.
//
// @example
// Lifetime lt = lifetime_begin(arena);
// (void)arena_alloc(lt.arena, MiB(1));
// lifetime_end(lt);
void lifetime_end(Lifetime self);

#ifdef BOOKSTORE_IMPLEMENTATION

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
