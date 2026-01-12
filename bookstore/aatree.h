/* aatree.h */
/* AA Trees */

#ifndef AATREE_H_
#define AATREE_H_

#include "./arena.h"
#include "./array.h"
#include "./basic.h"

typedef struct {
    i32 *index;
    bool visited;
} AATree__StackFrame;
ARRAY_TYPEDEF(AATree__StackFrame, AATree__Stack);
ARRAY_DECLARE_PREFIX(AATree__StackFrame, AATree__Stack, aatree__stack);

ARRAY_TYPEDEF(i32, AATree__WalkStack);
ARRAY_DECLARE_PREFIX(i32, AATree__WalkStack, aatree__walk_stack);

#define AANODE_TYPEDEF(T, name)                                                \
    typedef struct name {                                                      \
        i32 left_index, right_index, level;                                    \
        T value;                                                               \
    } name

#define AATREE_FIELDS(TNode) ARRAY_FIELDS(TNode)

#define AATREE_TYPEDEF(TNode, name)                                            \
    typedef struct name {                                                      \
        AATREE_FIELDS(TNode);                                                  \
        i32 root_index;                                                        \
    } name

#define AATREE_DECLARE(TValue, TNode, name)                                    \
    AATREE_DECLARE_PREFIX(TValue, TNode, name, name)

#define AATREE_DECLARE_PREFIX(TValue, TNode, name, prefix)                     \
    typedef struct {                                                           \
        void *user_data;                                                       \
        TValue value;                                                          \
    } name##WalkEntry;                                                         \
    typedef bool (*name##WalkVisitCallback)(name##WalkEntry entry);            \
    ARRAY_DECLARE_PREFIX(TNode, name, name##_);                                \
    name prefix##_new(Arena *arena, i32 max_insertions);                       \
    bool prefix##_insert(Arena *arena, name *self, TValue value);              \
    bool prefix##_delete(Arena *arena, name *self, TValue value);              \
    const TValue *prefix##_find(Arena *arena, name self, TValue value);        \
    bool prefix##_walk(Arena *arena, name self, name##WalkVisitCallback visit, \
                       void *user_data)

#define AATREE_DEFINE(TValue, TNode, name)                                     \
    AATREE_DEFINE_PREFIX(TValue, TNode, name, name)

#define AATREE_DEFINE_PREFIX(TValue, TNode, name, prefix)                      \
    AATREE_DEFINE_COMPLEX_PREFIX(TValue, TNode, COMPARE_BASIC, name, prefix)

#define AATREE_DEFINE_COMPLEX(TValue, TNode, compare, name)                    \
    AATREE_DEFINE_COMPLEX_PREFIX(TValue, TNode, compare, name, name)

#define TEMP__REPLACE_ME 64

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
    name prefix##_new(Arena *arena, i32 max_insertions) {                      \
        name self = name##__new(arena, max_insertions + 1);                    \
        TNode null_node = {0};                                                 \
        name##__push(&self, null_node);                                        \
        self.root_index = 0;                                                   \
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
                    *frame->index = self->count;                               \
                    TNode node = {                                             \
                        .value = value,                                        \
                        .level = 1,                                            \
                        .left_index = 0,                                       \
                        .right_index = 0,                                      \
                    };                                                         \
                    name##__push(self, node);                                  \
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

ARRAY_DEFINE_PREFIX(AATree__StackFrame, AATree__Stack, aatree__stack)
ARRAY_DEFINE_PREFIX(i32, AATree__WalkStack, aatree__walk_stack)

#endif // BOOKSTORE_IMPLEMENTATION

#endif // AATREE_H_
