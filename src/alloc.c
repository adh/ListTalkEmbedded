/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#include <ListTalkEmbedded/alloc.h>
#include <ListTalkEmbedded/error.h>

#include <stdlib.h>

void* LTE_malloc(size_t size){
    void* pointer = malloc(size == 0 ? 1 : size);

    if (pointer == NULL){
        LTE_error("Out of memory");
    }
    return pointer;
}

void* LTE_calloc(size_t count, size_t size){
    void* pointer = calloc(count == 0 ? 1 : count, size == 0 ? 1 : size);

    if (pointer == NULL){
        LTE_error("Out of memory");
    }
    return pointer;
}

void* LTE_realloc(void* pointer, size_t size){
    void* new_pointer = realloc(pointer, size == 0 ? 1 : size);

    if (new_pointer == NULL){
        LTE_error("Out of memory");
    }
    return new_pointer;
}

void LTE_free(void* pointer){
    free(pointer);
}
