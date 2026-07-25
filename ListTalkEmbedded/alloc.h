/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#ifndef H__ListTalkEmbedded__alloc__
#define H__ListTalkEmbedded__alloc__

#include <ListTalkEmbedded/macros/env_macros.h>

LTE__BEGIN_DECLS

void* LTE_malloc(size_t size);
void* LTE_calloc(size_t count, size_t size);
void* LTE_realloc(void* pointer, size_t size);
void LTE_free(void* pointer);

#define LTE_NEW(type) ((type*)LTE_calloc(1, sizeof(type)))

LTE__END_DECLS

#endif
