/* aatree.h */
/* AA Trees */

#ifndef AATREE_H_
#define AATREE_H_

#include "./arena.h"
#include "./array.h"
#include "./basic.h"
#include <stdbool.h>

#define TEMP__REPLACE_ME 64

// A `typedef` for a struct of a node in an AA tree with values of type `T`.
#define AANODE_TYPEDEF(T, name)                                                \
    typedef struct name {                                                      \
        i32 left_index, right_index, level;                                    \
        T value;                                                               \
    } name
// Define the necessary fields for an AA tree with nodes of type `TNode` (which
// should be defined with `AANODE_TYPEDEF`). Should be placed in a `struct`
// definition.
#define AATREE_FIELDS(TNode)                                                   \
    ARRAY_FIELDS(TNode);                                                       \
    i32 root_index;                                                            \
    i32 dangling_index
// A `typedef` for a struct with only the necessary fields for an AA tree with
// nodes of type `TNode` (which should be defined with `AANODE_TYPEDEF`), with
// the name `name`.
#define AATREE_TYPEDEF(TNode, name)                                            \
    typedef struct name {                                                      \
        AATREE_FIELDS(TNode);                                                  \
    } name
// Declare functions for an AA tree with values of type `TValue` and nodes of
// type `TNode`, named `name`.
//
// Prefix all the functions with `name` by default - use `AATREE_DECLARE_PREFIX`
// to manually supply a prefix.
//
// Can be placed in a header file.
#define AATREE_DECLARE(TValue, TNode, name)                                    \
    AATREE_DECLARE_PREFIX(TValue, TNode, name, name)
// Declare functions and helper types for an AA tree with values of type
// `TValue` and nodes of type `TNode`, named `name`.
//
// Prefix all the functions with `prefix`.
#define AATREE_DECLARE_PREFIX(TValue, TNode, name, prefix)                     \
    /* The entry passed to the `visit` callback for `prefix##_walk`. */        \
    typedef struct {                                                           \
        /*                                                                     \
         * The arena that was used to allocate the memory for the custom       \
         * recursion-like loop; can be used to make allocations in the `visit` \
         * callback itself. If the allocated memory does not need to live      \
         * beyond the callback, consider using a `Lifetime`.                   \
         */                                                                    \
        Arena *arena;                                                          \
        /* The `user_data` passed to `prefix##_walk`.  */                      \
        void *user_data;                                                       \
        /* The value of the node being visited. */                             \
        TValue value;                                                          \
    } name##WalkEntry;                                                         \
    /* The type of the `visit` callback passed to `prefix##_walk`. */          \
    typedef bool (*name##WalkVisitCallback)(name##WalkEntry entry);            \
    /*                                                                         \
     * Create a new AA tree with capacity for `capacity` nodes. Uses `arena`   \
     * to allocate the memory for the tree.                                    \
     */                                                                        \
    name prefix##_new(Arena *arena, i32 capacity);                             \
    /*                                                                         \
     * Insert the value `value` into the tree `self`. Uses `arena` to create a \
     * `Lifetime`, which is used to allocate the temporary stack for the       \
     * custom recursion-like loop.                                             \
     *                                                                         \
     * If the value is already in the tree, returns `true` and does not        \
     * reinsert. Otherwise, inserts the value and returns `false`.             \
     */                                                                        \
    bool prefix##_insert(Arena *arena, name *self, TValue value);              \
    /*                                                                         \
     * Delete the value `value` from the tree `self`. Uses `arena` to create a \
     * `Lifetime`, which is used to allocate the temporary stack for the       \
     * custom recursion-like loop.                                             \
     *                                                                         \
     * If the value is already in the tree, returns `true` and removes it.     \
     * Otherwise, returns `false` and does not modify the tree.                \
     */                                                                        \
    bool prefix##_delete(Arena *arena, name *self, TValue value);              \
    /*                                                                         \
     * Find the value `value` in the tree `self`. Uses `arena` to create a     \
     * `Lifetime`, which is used to allocate the temporary stack for the       \
     * custom recursion-like loop.                                             \
     *                                                                         \
     * If the value is in the tree, returns the pointer to it (this is useful  \
     * for cases where the comparison mechanism isn't a simple `==`, so values \
     * can be found by some sub-information within the `TValue`). Otherwise,   \
     * returns `NULL`.                                                         \
     */                                                                        \
    const TValue *prefix##_find(Arena *arena, name self, TValue value);        \
    /*                                                                         \
     * Walk the tree `self` in ascending order, calling `visit` on each of its \
     * nodes' values, passing `user_data` along to the `visit` callback. Uses  \
     * `arena` to allocate the temporary stack for the custom recursion-like   \
     * loop. If `visit` doesn't do any allocations, consider using a           \
     * `Lifetime` so the memory is deallocated after walking is finished.      \
     *                                                                         \
     * Returns `false` if `visit` returns `false` for any of the nodes.        \
     */                                                                        \
    bool prefix##_walk(Arena *arena, name self, name##WalkVisitCallback visit, \
                       void *user_data)
// Define functions for an AA tree with values of type `TValue` and nodes of
// type `TNode`, named `name`.
//
// Values of type `TValue` will be compared using `COMPARE_BASIC` - use
// `AATREE_DEFINE_COMPLEX` to manually supply comparison logic.
//
// Prefix all functions with `name` by default - use `AATREE_DEFINE_PREFIX` to
// manually supply a prefix.
//
// Companion to `AATREE_DECLARE`.
#define AATREE_DEFINE(TValue, TNode, name)                                     \
    AATREE_DEFINE_PREFIX(TValue, TNode, name, name)
// Define functions for an AA tree with values of type `TValue` and nodes of
// type `TNode`, named `name`.
//
// Values of type `TValue` will be compared using `COMPARE_BASIC` - use
// `AATREE_DEFINE_COMPLEX_PREFIX` to manually supply comparison logic.
//
// Prefix all the functions with `prefix`.
//
// Companion to `AATREE_DECLARE_PREFIX`.
#define AATREE_DEFINE_PREFIX(TValue, TNode, name, prefix)                      \
    AATREE_DEFINE_COMPLEX_PREFIX(TValue, TNode, COMPARE_BASIC, name, prefix)
// Define functions for an AA tree with values of type `TValue` and nodes of
// type `TNode`, named `name`.
//
// Values of type `TValue` will be compared using `compare`, which should return
// an `Order`.
//
// Prefix all functions with `name` by default - use
// `AATREE_DEFINE_COMPLEX_PREFIX` to manually supply a prefix.
//
// Companion to `AATREE_DEFINE`.
#define AATREE_DEFINE_COMPLEX(TValue, TNode, compare, name)                    \
    AATREE_DEFINE_COMPLEX_PREFIX(TValue, TNode, compare, name, name)
// Define functions for an AA tree with values of type `TValue` and nodes of
// type `TNode`, named `name`.
//
// Values of type `TValue` will be compared using `compare`, which should return
// an `Order`.
//
// Prefix all functions with `prefix`.
//
// Companion to `AATREE_DEFINE_PREFIX`.
#define AATREE_DEFINE_COMPLEX_PREFIX(TValue, TNode, compare, name, prefix)     \
    ARRAY_DEFINE_PREFIX(TNode, name, name##_)                                  \
    i32 prefix##__skew(name *self, i32 index) {                                \
        TNode *t = name##__get_ref(*self, index);                              \
        if (!t->left_index) return index;                                      \
        TNode *left = name##__get_ref(*self, t->left_index);                   \
        if (left->level != t->level) return index;                             \
        i32 ret = t->left_index;                                               \
        t->left_index = left->right_index;                                     \
        left->right_index = index;                                             \
        return ret;                                                            \
    }                                                                          \
    i32 prefix##__split(name *self, i32 index) {                               \
        TNode *t = name##__get_ref(*self, index);                              \
        TNode *right = name##__get_ref(*self, t->right_index);                 \
        if (!right->right_index ||                                             \
            t->level != name##__get(*self, right->right_index).level)          \
            return index;                                                      \
        i32 ret = t->right_index;                                              \
        t->right_index = right->left_index;                                    \
        right->left_index = index;                                             \
        right->level++;                                                        \
        return ret;                                                            \
    }                                                                          \
    void prefix##__decrease_level(name *self, i32 index) {                     \
        if (!index) return;                                                    \
        TNode *node = name##__get_ref(*self, index);                           \
        TNode left = name##__get(*self, node->left_index);                     \
        TNode *right = name##__get_ref(*self, node->right_index);              \
        i32 target_level = MIN(left.level, right->level) + 1;                  \
        if (target_level < node->level) {                                      \
            node->level = target_level;                                        \
            if (node->right_index && target_level < right->level)              \
                right->level = target_level;                                   \
        }                                                                      \
    }                                                                          \
    name prefix##_new(Arena *arena, i32 capacity) {                            \
        name self = name##__new(arena, capacity + 1);                          \
        TNode null_node = {0};                                                 \
        name##__push(&self, null_node);                                        \
        self.root_index = 0;                                                   \
        self.dangling_index = 0;                                               \
        return self;                                                           \
    }                                                                          \
    bool prefix##_insert(Arena *arena, name *self, TValue value) {             \
        Lifetime lt = lifetime_begin(arena);                                   \
        AATree__Stack stack = aatree__stack_new(lt.arena, TEMP__REPLACE_ME);   \
        AATree__StackFrame init = {.index = &self->root_index,                 \
                                   .visited = false};                          \
        aatree__stack_push(&stack, init);                                      \
        bool added = false;                                                    \
        while (stack.count) {                                                  \
            AATree__StackFrame *frame = aatree__stack_get_ref(stack, -1);      \
            if (!frame->visited) {                                             \
                frame->visited = true;                                         \
                if (added) continue;                                           \
                if (!*frame->index) {                                          \
                    TNode node = {                                             \
                        .value = value,                                        \
                        .level = 1,                                            \
                        .left_index = 0,                                       \
                        .right_index = 0,                                      \
                    };                                                         \
                    if (self->dangling_index) {                                \
                        i32 new_index = self->dangling_index;                  \
                        self->dangling_index =                                 \
                            self->items[new_index].left_index;                 \
                        *frame->index = new_index;                             \
                        self->items[new_index] = node;                         \
                    } else {                                                   \
                        *frame->index = self->count;                           \
                        name##__push(self, node);                              \
                    }                                                          \
                    added = true;                                              \
                    continue;                                                  \
                }                                                              \
                TNode *node = name##__get_ref(*self, *frame->index);           \
                Order ord = compare(value, node->value);                       \
                if (ord == ORDER_EQ) break;                                    \
                AATree__StackFrame new_frame = {0};                            \
                new_frame.index =                                              \
                    ord == ORDER_LT ? &node->left_index : &node->right_index;  \
                aatree__stack_push(&stack, new_frame);                         \
            } else {                                                           \
                aatree__stack_pop(&stack);                                     \
                *frame->index = prefix##__skew(self, *frame->index);           \
                *frame->index = prefix##__split(self, *frame->index);          \
            }                                                                  \
        }                                                                      \
        lifetime_end(lt);                                                      \
        return !added;                                                         \
    }                                                                          \
    bool prefix##_delete(Arena *arena, name *self, TValue value) {             \
        Lifetime lt = lifetime_begin(arena);                                   \
        AATree__Stack stack = aatree__stack_new(lt.arena, TEMP__REPLACE_ME);   \
        AATree__StackFrame init = {.index = &self->root_index,                 \
                                   .visited = false};                          \
        aatree__stack_push(&stack, init);                                      \
        i32 *last = &self->root_index;                                         \
        i32 deleted = 0;                                                       \
        bool found = false;                                                    \
        while (stack.count) {                                                  \
            AATree__StackFrame *frame = aatree__stack_get_ref(stack, -1);      \
            i32 index = *frame->index;                                         \
            if (!index) {                                                      \
                aatree__stack_pop(&stack);                                     \
                continue;                                                      \
            }                                                                  \
            TNode *node = name##__get_ref(*self, index);                       \
            if (!frame->visited) {                                             \
                frame->visited = true;                                         \
                last = frame->index;                                           \
                Order ord = compare(value, node->value);                       \
                AATree__StackFrame new_frame = {0};                            \
                if (ord == ORDER_LT) {                                         \
                    new_frame.index = &node->left_index;                       \
                    aatree__stack_push(&stack, new_frame);                     \
                } else {                                                       \
                    deleted = index;                                           \
                    new_frame.index = &node->right_index;                      \
                    aatree__stack_push(&stack, new_frame);                     \
                }                                                              \
            } else {                                                           \
                aatree__stack_pop(&stack);                                     \
                TNode *deleted_node = name##__get_ref(*self, deleted);         \
                if (index == *last && deleted &&                               \
                    compare(value, deleted_node->value) == ORDER_EQ) {         \
                    deleted_node->value = node->value;                         \
                    deleted = 0;                                               \
                    *last = node->right_index;                                 \
                    node->left_index = self->dangling_index;                   \
                    self->dangling_index = index;                              \
                    found = true;                                              \
                } else {                                                       \
                    TNode left = name##__get(*self, node->left_index);         \
                    TNode *right = name##__get_ref(*self, node->right_index);  \
                    i32 decreased = node->level - 1;                           \
                    if (left.level < decreased || right->level < decreased) {  \
                        if (right->level < --node->level) {                    \
                            right->level = node->level;                        \
                        }                                                      \
                        index = prefix##__skew(self, index);                   \
                        *frame->index = index;                                 \
                        node = name##__get_ref(*self, index);                  \
                        node->right_index =                                    \
                            prefix##__skew(self, node->right_index);           \
                        right = name##__get_ref(*self, node->right_index);     \
                        right->right_index =                                   \
                            prefix##__skew(self, right->right_index);          \
                        index = prefix##__split(self, index);                  \
                        *frame->index = index;                                 \
                        node = name##__get_ref(*self, index);                  \
                        node->right_index =                                    \
                            prefix##__split(self, node->right_index);          \
                    }                                                          \
                }                                                              \
            }                                                                  \
        }                                                                      \
        lifetime_end(lt);                                                      \
        return found;                                                          \
    }                                                                          \
    const TValue *prefix##_find(Arena *arena, name self, TValue value) {       \
        Lifetime lt = lifetime_begin(arena);                                   \
        AATree__WalkStack stack =                                              \
            aatree__walk_stack_new(lt.arena, TEMP__REPLACE_ME);                \
        aatree__walk_stack_push(&stack, self.root_index);                      \
        const TValue *found = NULL;                                            \
        while (stack.count) {                                                  \
            i32 index = aatree__walk_stack_pop(&stack);                        \
            if (!index) continue;                                              \
            TNode *node = name##__get_ref(self, index);                        \
            Order ord = compare(value, node->value);                           \
            if (ord == ORDER_GT) {                                             \
                aatree__walk_stack_push(&stack, node->right_index);            \
            } else if (ord == ORDER_LT) {                                      \
                aatree__walk_stack_push(&stack, node->left_index);             \
            } else {                                                           \
                found = &node->value;                                          \
                break;                                                         \
            }                                                                  \
        }                                                                      \
        lifetime_end(lt);                                                      \
        return found;                                                          \
    }                                                                          \
    bool prefix##_walk(Arena *arena, name self, name##WalkVisitCallback visit, \
                       void *user_data) {                                      \
        AATree__WalkStack stack =                                              \
            aatree__walk_stack_new(arena, TEMP__REPLACE_ME);                   \
        aatree__walk_stack_push(&stack, self.root_index);                      \
        while (stack.count) {                                                  \
            i32 index = aatree__walk_stack_pop(&stack);                        \
            if (!index) continue;                                              \
            if (index > 0) {                                                   \
                TNode node = name##__get(self, index);                         \
                aatree__walk_stack_push(&stack, -index);                       \
                aatree__walk_stack_push(&stack, node.left_index);              \
            } else {                                                           \
                TNode node = name##__get(self, -index);                        \
                name##WalkEntry entry = {.user_data = user_data,               \
                                         .value = node.value};                 \
                if (!visit(entry)) return false;                               \
                aatree__walk_stack_push(&stack, node.right_index);             \
            }                                                                  \
        }                                                                      \
        return true;                                                           \
    }

#ifdef BOOKSTORE_IMPLEMENTATION

typedef struct {
    i32 *index;
    bool visited;
} AATree__StackFrame;
ARRAY_TYPEDEF(AATree__StackFrame, AATree__Stack);
ARRAY_DEFINE_PREFIX(AATree__StackFrame, AATree__Stack, aatree__stack)

ARRAY_TYPEDEF(i32, AATree__WalkStack);
ARRAY_DEFINE_PREFIX(i32, AATree__WalkStack, aatree__walk_stack)

#endif // BOOKSTORE_IMPLEMENTATION

#endif // AATREE_H_
