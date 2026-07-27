/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Ales Hakl
 */

#ifndef H__ListTalkEmbedded__Class__
#define H__ListTalkEmbedded__Class__

#include <ListTalkEmbedded/macros/env_macros.h>
#include <ListTalkEmbedded/error.h>
#include <ListTalkEmbedded/utils.h>
#include <ListTalkEmbedded/value.h>

LTE__BEGIN_DECLS

typedef struct LTE_Class_s LTE_Class;

typedef char*(*LTE_Class_debugPrint_Func)(LTE_Value self);
typedef size_t(*LTE_Class_hash_Func)(LTE_Value self);
typedef int(*LTE_Class_equal_p_Func)(LTE_Value self, LTE_Value other);
typedef void (*LTE_Class_finalize_Func)(LTE_Value self);
typedef void (*LTE_Class_mark_Func)(LTE_Value self);

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

#define LTE_CLASS_FLAG_FLEXIBLE    1
#define LTE_CLASS_FLAG_SPECIAL     2
#define LTE_CLASS_FLAG_ABSTRACT    4
#define LTE_CLASS_FLAG_FINAL       8
#define LTE_CLASS_FLAG_IMMUTABLE  16
#define LTE_CLASS_FLAG_SCALAR     32
#define LTE_CLASS_FLAG_ALLOCATABLE 64

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
    LTE_Class_debugPrint_Func debugPrint;
    LTE_Class_hash_Func hash;
    LTE_Class_equal_p_Func equal_p;
    LTE_Class_finalize_Func finalize;
    LTE_Class_mark_Func mark;
    LTE_Slot_Descriptor* slots;
    LTE_Method_Descriptor* methods;
    LTE_Method_Descriptor* class_methods;
} LTE_Class_Descriptor;

struct LTE_Class_s {
    LTE_Object base;
    LTE_Class** superclasses; /* NULL terminated */
    LTE_Value precedence_list; /* List */
    size_t instance_size;
    unsigned int class_flags;
    size_t slot_count;
    LTE_Class_Slot* slots;
    LTE_Value methods;
    LTE_Value method_cache;
    uintptr_t cache_version;
    LTE_Value name;
    LTE_Class_debugPrint_Func debugPrint;
    LTE_Class_hash_Func hash;
    LTE_Class_equal_p_Func equal_p;
    LTE_Class_finalize_Func finalize;
    LTE_Class_mark_Func mark;
    LTE_Value documentation;
    LTE_Class_Descriptor* native_descriptor; /* Native class descriptor */
};

#define LTE_STATIC_CLASS(name) ((LTE_Value)(uintptr_t)&name##_class)

#define LTE_DECLARE_CLASS(name) \
    typedef struct name##_s name; \
    extern LTE_Class name##_class; \
    extern LTE_Class name##_class_class; \
    static inline int name##_p(LTE_Value value){ \
        return LTE_Value_class(value) == &name##_class; \
    } \
    static inline name* name##_from_value(LTE_Value value){ \
        if (!name##_p(value)){ \
            LTE_error("Unexpected object class"); \
        } \
        return (name*)(uintptr_t)value; \
    }

#define LTE_DEFINE_CLASS(c_name) \
    static LTE_Class_Descriptor c_name##_class_descriptor; \
    LTE_Class c_name##_class_class = { \
        .base = {.cls = (uintptr_t)&LTE_Class_class_class}, \
        .instance_size = sizeof(LTE_Class), \
    }; \
    LTE_Class c_name##_class = { \
        .base = {.cls = (uintptr_t)&c_name##_class_class}, \
        .native_descriptor = &c_name##_class_descriptor, \
    }; \
    static void LTE___init_##c_name(void){ \
        LTE_init_native_class(&c_name##_class); \
    }; \
    LTE_REGISTER_CONSTRUCTOR(LTE___init_##c_name) \
    static LTE_Class_Descriptor c_name##_class_descriptor =

extern LTE_Class LTE_Object_class;
extern LTE_Class LTE_Object_class_class;
extern LTE_Class LTE_Class_class;
extern LTE_Class LTE_Class_class_class;

void LTE_init_native_class(LTE_Class* klass);
void* LTE_Class_alloc(LTE_Class* klass);
void* LTE_Class_alloc_flexible(LTE_Class* klass, size_t flex);

#define LTE_Class_ALLOC(type) (type*)(LTE_Class_alloc(&type##_class))
#define LTE_Class_ALLOC_FLEXIBLE(type, flex) \
    (type*)(LTE_Class_alloc_flexible(&type##_class, flex))

LTE__END_DECLS

#endif
