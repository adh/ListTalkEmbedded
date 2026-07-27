/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ales Hakl
 */

#ifndef H__ListTalkEmbedded__Class__
#define H__ListTalkEmbedded__Class__

#include <ListTalkEmbedded/macros/env_macros.h>
#include <ListTalkEmbedded/value.h>

LTE__BEGIN_DECLS

typedef struct LTE_Class_s LTE_Class;

typedef char*(*LTE_Class_debugPrint)(LTE_Value self);
typedef size_t(*LTE_Class_hash_Func)(LTE_Value self);
typedef int(*LTE_Class_equal_p_Func)(LTE_Value self, LTE_Value other);

typedef struct LTE_SlotType_s LTE_SlotType;
typedef struct LTE_Primitive_s LTE_Primitive;

typedef struct LTE_Class_Slot {
    LTE_Value name;
    size_t offset;
    LTE_SlotType* type;
} LTE_Class_Slot;

struct LTE_SlotType_s {
    LTE_Value (*ref)(LTE_Class_Slot* slot, LTE_Value object);
    void (*set)(LTE_Class_Slot* slot, LTE_Value object, LTE_Value value);
};

extern LTE_SlotType LTE_SlotType_Object;
extern LTE_SlotType LTE_SlotType_ReadonlyObject;

typedef struct LTE_Slot_Descriptor {
    char* name;
    size_t offset;
    LTE_SlotType* type;
} LTE_Slot_Descriptor;

#define LTE_NULL_NATIVE_CLASS_SLOT_DESCRIPTOR {NULL, 0, NULL}
typedef struct LTE_Method_Descriptor {
    char* selector;
    LTE_Primitive* primitive;
} LTE_Method_Descriptor;

#define LTE_NULL_NATIVE_CLASS_METHOD_DESCRIPTOR {NULL, NULL}
typedef struct LTE_Class_Descriptor_s {
    LTE_Class* superclass;
    LTE_Class* metaclass_superclass;
    char* package;
    char* name;
    char* documentation;
    size_t instance_size;
    int class_flags;
    LTE_Class_debugPrint debugPrint;
    LTE_Class_hash_Func hash;
    LTE_Class_equal_p_Func equal_p;
    LTE_Slot_Descriptor* slots;
    LTE_Method_Descriptor* methods;
    LTE_Method_Descriptor* class_methods;
} LTE_Class_Descriptor;

LTE__END_DECLS

#endif