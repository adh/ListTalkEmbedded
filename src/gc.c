/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ales Hakl
 */

#include <ListTalkEmbedded/gc.h>
#include <ListTalkEmbedded/alloc.h>
#include <ListTalkEmbedded/Class.h>
#include <ListTalkEmbedded/error.h>

#include <string.h>

static uintptr_t* stack_start = NULL;
static unsigned char* heap_start = NULL;
static size_t heap_size = 0;
static size_t allocated_since_last_gc = 0;

enum {
    LTE_GC_ALIGNMENT_WORDS = 4,
};

#define LTE_GC_ALIGNMENT (sizeof(uintptr_t) * LTE_GC_ALIGNMENT_WORDS)

typedef struct FreeListNode_s {
    uintptr_t header;
    size_t size; /* bytes, including this header */
    struct FreeListNode_s* next;
} FreeListNode;

static FreeListNode* free_list_head = NULL;

static size_t align_size(size_t size){
    return (size + LTE_GC_ALIGNMENT - 1) & ~(LTE_GC_ALIGNMENT - 1);
}

static size_t class_instance_size(LTE_Class* cls){
    size_t size;

    if (cls == NULL || cls->instance_size < sizeof(LTE_Object)){
        LTE_error("Invalid class instance size");
    }

    size = align_size(cls->instance_size);
    if (size < sizeof(FreeListNode)){
        size = align_size(sizeof(FreeListNode));
    }
    return size;
}

static int heap_contains_pointer(uintptr_t value){
    return heap_start != NULL
        && value >= (uintptr_t)heap_start
        && value < (uintptr_t)heap_start + heap_size
        && (value & LTE_VALUE_TAG_MASK) == 0;
}

void LTE_gc_init(uintptr_t* start, size_t size){
    heap_start = (unsigned char*)start;
    heap_size = size & ~(LTE_GC_ALIGNMENT - 1);
    stack_start = (uintptr_t*)__builtin_frame_address(1);

    if (heap_size < align_size(sizeof(FreeListNode))){
        LTE_error("GC heap is too small");
    }

    FreeListNode* initial_node = (FreeListNode*)heap_start;
    initial_node->header = 0;
    initial_node->size = heap_size;
    initial_node->next = NULL;
    free_list_head = initial_node;
    allocated_since_last_gc = 0;
}

static void mark_object(LTE_Value object){
    if(object == LTE_NIL || object == LTE_INVALID || !heap_contains_pointer(object)){
        return;
    }

    LTE_Object* obj = (LTE_Object*)object;
    if(obj->cls == 0){
        return;
    }
    if(obj->cls & LTE_VALUE_MARK_BIT){
        return; /* Already marked */
    }
    obj->cls |= LTE_VALUE_MARK_BIT;

    LTE_Class* cls = LTE_Object_class(object);
    mark_object((LTE_Value)cls);
    /* TODO: do not mark primitives and special forms. */
    if(cls->mark){
        cls->mark(object);
    }
}

static void append_free_node(FreeListNode** tail, FreeListNode* node){
    node->next = NULL;
    if (*tail == NULL){
        free_list_head = node;
    } else {
        (*tail)->next = node;
    }
    *tail = node;
}

static void sweep(void){
    unsigned char* current = heap_start;
    unsigned char* heap_end = heap_start + heap_size;
    FreeListNode* tail = NULL;

    free_list_head = NULL;
    while(current < heap_end){
        uintptr_t header = *(uintptr_t*)current;
        size_t size;

        if(header == 0){
            FreeListNode* free_node = (FreeListNode*)current;

            size = free_node->size;
            if (size == 0 || current + size > heap_end){
                LTE_error("Corrupt GC free block");
            }
        } else {
            LTE_Object* object = (LTE_Object*)current;

            LTE_Class* cls = LTE_Object_class((LTE_Value)object);

            size = class_instance_size(cls);
            if (current + size > heap_end){
                LTE_error("Corrupt GC object");
            }

            if (header & LTE_VALUE_MARK_BIT){
                object->cls = header & ~LTE_VALUE_MARK_BIT;
                current += size;
                continue;
            }

            if (cls->finalize != NULL){
                cls->finalize((LTE_Value)object);
            }

            FreeListNode* free_node = (FreeListNode*)current;
            free_node->header = 0;
            free_node->size = size;
            free_node->next = NULL;
        }

        FreeListNode* free_node = (FreeListNode*)current;
        unsigned char* next = current + size;

        while(next < heap_end && *(uintptr_t*)next == 0){
            FreeListNode* next_free = (FreeListNode*)next;

            if (next_free->size == 0 || next + next_free->size > heap_end){
                LTE_error("Corrupt GC free block");
            }
            free_node->size += next_free->size;
            next += next_free->size;
        }

        append_free_node(&tail, free_node);
        current = next;
    }
}

struct RootNode_s {
    void* root;
    struct RootNode_s* next;
};
static struct RootNode_s* root_list_head = NULL;

static void mark_stack(uintptr_t* start, uintptr_t* end){
    uintptr_t* lower = start < end ? start : end;
    uintptr_t* upper = start < end ? end : start;

    for(uintptr_t* ptr = lower; ptr < upper; ++ptr){
        mark_object((LTE_Value)(*ptr));
    }
}

void LTE_gc_collect(void){
    struct RootNode_s* current_root = root_list_head;
    while(current_root != NULL){
        mark_object(*(LTE_Value*)(current_root->root));
        current_root = current_root->next;
    }
    uintptr_t* stack_end = (uintptr_t*)__builtin_frame_address(0);
    mark_stack(stack_start, stack_end);
    sweep();
    allocated_since_last_gc = 0;
}
void LTE_gc_register_root(void* root){
    struct RootNode_s* new_node = LTE_NEW(struct RootNode_s);
    new_node->root = root;
    new_node->next = root_list_head;
    root_list_head = new_node;
}
void LTE_gc_unregister_root(void* root){
    struct RootNode_s* current = root_list_head;
    struct RootNode_s* prev = NULL;

    while(current != NULL){
        if(current->root == root){
            if(prev == NULL){
                root_list_head = current->next;
            } else {
                prev->next = current->next;
            }
            LTE_free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

static void* allocate_object(size_t size){
    FreeListNode* prev = NULL;
    FreeListNode* current = free_list_head;

    while(current != NULL){
        if(current->size >= size){
            if(current->size > size + sizeof(FreeListNode)){
                FreeListNode* new_node = (FreeListNode*)((unsigned char*)current + size);
                new_node->header = 0;
                new_node->size = current->size - size;
                new_node->next = current->next;

                if(prev == NULL){
                    free_list_head = new_node;
                } else {
                    prev->next = new_node;
                }
            } else {
                if(prev == NULL){
                    free_list_head = current->next;
                } else {
                    prev->next = current->next;
                }
            }
            return current;
        }
        prev = current;
        current = current->next;
    }
    return NULL; /* Out of memory */
}

LTE_Value LTE_gc_alloc(LTE_Class* cls){
    if (allocated_since_last_gc > heap_size / 2) {
        LTE_gc_collect();
    }

    size_t object_size = class_instance_size(cls);
    LTE_Object* object = allocate_object(object_size);
    if(object == NULL){
        LTE_gc_collect();
        object = allocate_object(object_size);
        if (object == NULL) {
            return LTE_INVALID; /* Out of memory */
        }
    }
    memset(object, 0, object_size);
    object->cls = (uintptr_t)cls;
    allocated_since_last_gc += object_size;
    return (LTE_Value)object;
}

void LTE_gc_mark_object(LTE_Value object){
    mark_object(object);
}
void LTE_gc_mark_objects(LTE_Value* objects, size_t count){
    for(size_t i = 0; i < count; ++i){
        mark_object(objects[i]);
    }
}
