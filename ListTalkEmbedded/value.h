/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ales Hakl
 */

#ifndef H__ListTalkEmbedded__value__
#define H__ListTalkEmbedded__value__

#include <ListTalkEmbedded/macros/env_macros.h>

LTE__BEGIN_DECLS

typedef uintptr_t LTE_Value;
typedef struct LTE_Class_s LTE_Class;

typedef struct LTE_Object_s {
    uintptr_t cls; /* Pointer to LTE_Class, but with LSB used for mark bit. */
} LTE_Object;

#define LTE_INVALID ((LTE_Value)0)
#define LTE_NIL     ((LTE_Value)0xfffffffc)

#define LTE_VALUE_TAG_MASK ((uintptr_t)0x3)
#define LTE_VALUE_MARK_BIT ((uintptr_t)0x1)

static inline LTE_Class* LTE_Object_class(LTE_Value object){
    return (LTE_Class*)(((LTE_Object*)object)->cls & ~LTE_VALUE_MARK_BIT);
}

LTE__END_DECLS

#endif
