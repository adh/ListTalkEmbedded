/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ales Hakl
 */

#include <ListTalkEmbedded/gc.h>
#include <ListTalkEmbedded/Class.h>

static uintptr_t* stack_start = NULL;
static uintptr_t* heap_start = NULL;
static size_t heap_size = 0;
static size_t allocated_since_last_gc = 0;

typedef struct FreeListNode_s {
    uintptr_t header;
    uintptr_t size; /* including header */
    struct FreeListNode_s* next;
} FreeListNode;

static FreeListNode* free_list_head = NULL;

void LTE_gc_init(uintptr_t* start, size_t size){
    heap_start = start;
    heap_size = size;
    stack_start = (uintptr_t*)__builtin_frame_address(1);
    FreeListNode* initial_node = (FreeListNode*)heap_start;
    initial_node->header = 0; /* mark bit is 0 */
    initial_node->size = heap_size;
    initial_node->next = NULL;
    free_list_head = initial_node;
}

static void mark_object(LTE_Value object){
    if(object == LTE_NIL || object == LTE_INVALID){
        return;
    }
    if (object & 3) {
        return; /* Not a pointer to an object */
    }
    LTE_Object* obj = (LTE_Object*)object;
    if(obj->cls & 1){
        return; /* Already marked */
    }
    obj->cls |= 1; /* Set mark bit */
    LTE_Class* cls = (LTE_Class*)(obj->cls & ~((uintptr_t)1));
    // TODO: do not mark primitives and special forms
    if(cls->mark){
        cls->mark(object);
    }
}
static void sweep(){
    FreeListNode* current = (FreeListNode*)heap_start;
    FreeListNode* prev = NULL;

    while((uintptr_t)current < (uintptr_t)heap_start + heap_size){
        if(current->header & 1){
            current->header &= ~((uintptr_t)1); /* Clear mark bit */
            prev = current;
            current = (FreeListNode*)((uintptr_t)current + current->size);
        } else {
            if(prev == NULL){
                free_list_head = current;
            } else {
                prev->next = current;
            }
            FreeListNode* next_node = (FreeListNode*)((uintptr_t)current + current->size);
            current->next = next_node;
            current = next_node;
        }
    }
}

struct RootNode_s {
    void* root;
    struct RootNode_s* next;
};
static struct RootNode_s* root_list_head = NULL;

void mark_stack(uintptr_t* stack_start, uintptr_t* stack_end){
    for(uintptr_t* ptr = stack_end; ptr < stack_start; ++ptr){
        if (*ptr >= (uintptr_t)heap_start && *ptr < (uintptr_t)heap_start + heap_size) {
            mark_object((LTE_Value)(*ptr));
        }
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
    struct RootNode_s* new_node = (struct RootNode_s*)malloc(sizeof(struct RootNode_s));
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
            free(current);
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
                FreeListNode* new_node = (FreeListNode*)((uintptr_t)current + size);
                new_node->header = 0; /* mark bit is 0 */
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
            return (void*)((uintptr_t)current + sizeof(uintptr_t));
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

    size_t object_size = cls->instance_size;
    object_size = (object_size + sizeof(FreeListNode) - 1) & ~(sizeof(FreeListNode) - 1); /* Align to FreeListNode size */
    LTE_Object* object = allocate_object(object_size + sizeof(uintptr_t));
    if(object == NULL){
        LTE_gc_collect();
        object = allocate_object(object_size + sizeof(uintptr_t));
        if (object == NULL) {
            return LTE_INVALID; /* Out of memory */
        }
    }
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
