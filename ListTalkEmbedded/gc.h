/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#ifndef H__ListTalkEmbedded__gc__
#define H__ListTalkEmbedded__gc__


#include <ListTalkEmbedded/macros/env_macros.h>
#include <ListTalkEmbedded/value.h>

LTE__BEGIN_DECLS

void LTE_gc_init(uintptr_t* heap_start, size_t heap_size);
void LTE_gc_collect(void);
void LTE_gc_register_root(void* root);
void LTE_gc_unregister_root(void* root);
LTE_Value LTE_gc_alloc(LTE_Class* cls);

LTE__END_DECLS

#endif