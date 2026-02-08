#ifndef HASHMAP_H_
#define HASHMAP_H_

#include "./arena.h"
#include "./array.h"
#include "./basic.h"
#include "./string.h"
#include <string.h>

#define HASHMAP_FOREACH(map, key, T_value, block)                              \
    for (i32 hashmap__i = 0; hashmap__i < (map).capacity; hashmap__i++) {      \
        if ((map).entries[hashmap__i].key.count < 0) continue;                 \
        StringView key = (map).entries[hashmap__i].key;                        \
        T_value = (map).entries[hashmap__i].value;                             \
        do block while (0);                                                    \
        i32 hashmap__j = (map).entries[hashmap__i].next_index;                 \
        while (hashmap__j >= 0) {                                              \
            StringView key = (map).buckets.items[hashmap__j].key;              \
            T_value = (map).buckets.items[hashmap__j].value;                   \
            do block while (0);                                                \
            hashmap__j = (map).buckets.items[hashmap__j].next_index;           \
        }                                                                      \
    }

#define HASHMAP_TYPEDEF(T, name)                                               \
    typedef struct name##Entry {                                               \
        StringView key;                                                        \
        T value;                                                               \
        i32 next_index;                                                        \
    } name##Entry;                                                             \
    ARRAY_TYPEDEF(name##Entry, name##__Buckets);                               \
    typedef struct name {                                                      \
        name##__Buckets buckets;                                               \
        name##Entry *entries;                                                  \
        i32 capacity;                                                          \
        i32 dangling_index;                                                    \
    } name

#define HASHMAP_DECLARE(T, name) HASHMAP_DECLARE_PREFIX(T, name, name)

#define HASHMAP_DECLARE_PREFIX(T, name, prefix)                                \
    name prefix##_new(Arena *arena, i32 capacity);                             \
    bool prefix##_set(name *self, StringView key, T value);                    \
    bool prefix##_get(name self, StringView key, T *out);                      \
    bool prefix##_delete(name *self, StringView key)

#define HASHMAP_DEFINE(T, name) HASHMAP_DEFINE_PREFIX(T, name, name)

#define HASHMAP_DEFINE_PREFIX(T, name, prefix)                                 \
    ARRAY_DEFINE(name##Entry, name##__Buckets);                                \
    name prefix##_new(Arena *arena, i32 capacity) {                            \
        name ret = {0};                                                        \
        ret.entries = arena_alloc(arena, capacity * sizeof(name##Entry));      \
        for (i32 i = 0; i < capacity; i++) {                                   \
            ret.entries[i].key = SV_INVALID;                                   \
            ret.entries[i].next_index = -1;                                    \
            ret.entries[i].value = (T){0};                                     \
        }                                                                      \
        ret.buckets = name##__Buckets_new(arena, capacity / 2);                \
        ret.capacity = capacity;                                               \
        ret.dangling_index = -1;                                               \
        return ret;                                                            \
    }                                                                          \
    bool prefix##_set(name *self, StringView key, T value) {                   \
        i32 index = hashmap__fnv32hash(key) % self->capacity;                  \
        name##Entry *entry = &self->entries[index];                            \
                                                                               \
        name##Entry new_entry = {                                              \
            .next_index = -1,                                                  \
            .key = key,                                                        \
            .value = value,                                                    \
        };                                                                     \
                                                                               \
        if (entry->key.count < 0) {                                            \
            *entry = new_entry;                                                \
            return false;                                                      \
        }                                                                      \
                                                                               \
        if (sv_eq(key, entry->key)) {                                          \
            entry->value = value;                                              \
            return true;                                                       \
        }                                                                      \
        while (entry->next_index >= 0) {                                       \
            entry = name##__Buckets_get_ref(self->buckets, entry->next_index); \
            if (sv_eq(key, entry->key)) {                                      \
                entry->value = value;                                          \
                return true;                                                   \
            };                                                                 \
        }                                                                      \
                                                                               \
        if (self->dangling_index >= 0) {                                       \
            i32 new_index = self->dangling_index;                              \
            name##Entry *dangling =                                            \
                name##__Buckets_get_ref(self->buckets, new_index);             \
            self->dangling_index = dangling->next_index;                       \
            entry->next_index = new_index;                                     \
            *dangling = new_entry;                                             \
        } else {                                                               \
            entry->next_index = self->buckets.count;                           \
            name##__Buckets_push(&self->buckets, new_entry);                   \
        }                                                                      \
        return false;                                                          \
    }                                                                          \
    bool prefix##_get(name self, StringView key, T *out) {                     \
        i32 index = hashmap__fnv32hash(key) % self.capacity;                   \
                                                                               \
        name##Entry entry = self.entries[index];                               \
        while (!sv_eq(entry.key, key)) {                                       \
            if (entry.next_index < 0) return false;                            \
            entry = name##__Buckets_get(self.buckets, entry.next_index);       \
        }                                                                      \
        *out = entry.value;                                                    \
        return true;                                                           \
    }                                                                          \
    bool prefix##_delete(name *self, StringView key) {                         \
        i32 index = hashmap__fnv32hash(key) % self->capacity;                  \
                                                                               \
        name##Entry *entry = &self->entries[index];                            \
        name##Entry *prev = entry;                                             \
        while (!sv_eq(entry->key, key)) {                                      \
            if (entry->key.count < 0) return false;                            \
            prev = entry;                                                      \
            entry = name##__Buckets_get_ref(self->buckets, entry->next_index); \
        }                                                                      \
        if (prev == entry) {                                                   \
            if (entry->next_index < 0) {                                       \
                entry->key = SV_INVALID;                                       \
                return true;                                                   \
            }                                                                  \
                                                                               \
            i32 new_index = self->dangling_index;                              \
            name##Entry *next =                                                \
                name##__Buckets_get_ref(self->buckets, entry->next_index);     \
            self->dangling_index = entry->next_index;                          \
            *entry = *next;                                                    \
            next->next_index = new_index;                                      \
            next->key = SV_INVALID;                                            \
            return true;                                                       \
        }                                                                      \
                                                                               \
        i32 removed_index = prev->next_index;                                  \
        prev->next_index = entry->next_index;                                  \
        entry->next_index = self->dangling_index;                              \
        entry->key = SV_INVALID;                                               \
        self->dangling_index = removed_index;                                  \
                                                                               \
        return true;                                                           \
    }

#ifdef BOOKSTORE_IMPLEMENTATION

#define HASHMAP__FNV32_PRIME 0x01000193
#define HASHMAP__FNV32_BASE  0x811C9DC5

u32 hashmap__fnv32hash(StringView in) {
    u32 ret = HASHMAP__FNV32_BASE;
    while (in.count) ret = HASHMAP__FNV32_PRIME * (ret ^ sv_shift(&in));
    return ret;
}

#endif // BOOKSTORE_IMPLEMENTATION

#endif // HASHMAP_H_
