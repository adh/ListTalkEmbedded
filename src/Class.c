/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#include <ListTalkEmbedded/Class.h>
#include <ListTalkEmbedded/Primitive.h>
#include <ListTalkEmbedded/gc.h>
#include <ListTalkEmbedded/utils.h>

#include <stdlib.h>
#include <string.h>

static LTE_Value object_slot_ref(LTE_Class_Slot* slot, LTE_Value object){
    LTE_Value* val = (LTE_Value*)((uint8_t*)(uintptr_t)object + slot->offset);
    return *val;
}

static void object_slot_set(LTE_Class_Slot* slot,
                            LTE_Value object,
                            LTE_Value value){
    LTE_Value* val = (LTE_Value*)((uint8_t*)(uintptr_t)object + slot->offset);
    *val = value;
}

static void readonly_object_slot_set(LTE_Class_Slot* slot,
                                     LTE_Value object,
                                     LTE_Value value){
    (void)slot;
    (void)object;
    (void)value;
    LTE_error("Readonly slot");
}

LTE_SlotType LTE_SlotType_Object = {
    .ref = object_slot_ref,
    .set = object_slot_set,
};

LTE_SlotType LTE_SlotType_ReadonlyObject = {
    .ref = object_slot_ref,
    .set = readonly_object_slot_set,
};

static size_t Class_default_hash(LTE_Value obj){
    return LTE_pointer_hash((void*)(uintptr_t)obj);
}

static int Class_default_equal_p(LTE_Value left, LTE_Value right){
    return left == right;
}

static char* Class_debugPrint(LTE_Value obj){
    LTE_Class* klass = (LTE_Class*)(uintptr_t)obj;

    if (klass->native_descriptor != NULL && klass->native_descriptor->name != NULL){
        return LTE_sprintf("#<Class %s>", klass->native_descriptor->name);
    }
    if (klass->name != LTE_NIL && klass->name != LTE_INVALID){
        return LTE_sprintf("#<Class %s>", (char*)(uintptr_t)klass->name);
    }
    return LTE_sprintf("#<Class 0x%zx>", (size_t)(uintptr_t)klass);
}

static LTE_Class_Slot Class_slots[] = {
    {(LTE_Value)(uintptr_t)"name", offsetof(LTE_Class, name), &LTE_SlotType_Object},
    {(LTE_Value)(uintptr_t)"methods", offsetof(LTE_Class, methods), &LTE_SlotType_Object},
    {(LTE_Value)(uintptr_t)"method-cache", offsetof(LTE_Class, method_cache), &LTE_SlotType_Object},
    {(LTE_Value)(uintptr_t)"documentation", offsetof(LTE_Class, documentation), &LTE_SlotType_Object},
};

LTE_Class LTE_Class_class_class = {
    .base = {.cls = (uintptr_t)&LTE_Class_class_class},
    .instance_size = sizeof(LTE_Class),
};

LTE_Class LTE_Object_class_class = {
    .base = {.cls = (uintptr_t)&LTE_Class_class_class},
    .instance_size = sizeof(LTE_Class),
};

LTE_Class LTE_Object_class = {
    .base = {.cls = (uintptr_t)&LTE_Object_class_class},
    .instance_size = sizeof(LTE_Object),
    .native_descriptor = NULL,
};

LTE_Class LTE_Class_class = {
    .base = {.cls = (uintptr_t)&LTE_Class_class_class},
    .instance_size = sizeof(LTE_Class),
    .native_descriptor = NULL,
};

static LTE_Class* empty_superclasses[] = {NULL};
static LTE_Class* object_superclasses[] = {&LTE_Object_class, NULL};
static LTE_Class* class_superclasses[] = {&LTE_Class_class, NULL};

static int core_class_cycle_finalized = 0;

static LTE_Class** make_single_superclass_list(LTE_Class* superclass){
    LTE_Class** superclasses;

    if (superclass == NULL){
        superclasses = LTE_malloc(sizeof(LTE_Class*));
        superclasses[0] = NULL;
        return superclasses;
    }

    superclasses = LTE_malloc(sizeof(LTE_Class*) * 2);
    superclasses[0] = superclass;
    superclasses[1] = NULL;
    return superclasses;
}

static size_t count_slot_descriptors(LTE_Slot_Descriptor* descriptor_slots){
    size_t count = 0;

    if (descriptor_slots == NULL){
        return 0;
    }
    while (descriptor_slots[count].name != NULL){
        count++;
    }
    return count;
}

static int compare_slots_by_name(const void* left, const void* right){
    const LTE_Class_Slot* left_slot = (const LTE_Class_Slot*)left;
    const LTE_Class_Slot* right_slot = (const LTE_Class_Slot*)right;

    if (left_slot->name < right_slot->name){
        return -1;
    }
    if (left_slot->name > right_slot->name){
        return 1;
    }
    return 0;
}

static void materialize_slots(LTE_Class* klass,
                              LTE_Slot_Descriptor* descriptor_slots){
    size_t superclass_slot_count = 0;
    size_t descriptor_slot_count;
    size_t max_slot_count;
    size_t slot_count = 0;
    LTE_Class_Slot* slots;
    size_t i;

    if (klass->superclasses != NULL && klass->superclasses[0] != NULL){
        superclass_slot_count = klass->superclasses[0]->slot_count;
    }
    descriptor_slot_count = count_slot_descriptors(descriptor_slots);
    max_slot_count = superclass_slot_count + descriptor_slot_count;

    if (max_slot_count == 0){
        klass->slot_count = 0;
        klass->slots = NULL;
        return;
    }

    slots = LTE_malloc(sizeof(LTE_Class_Slot) * max_slot_count);
    if (superclass_slot_count != 0){
        memcpy(slots,
               klass->superclasses[0]->slots,
               sizeof(LTE_Class_Slot) * superclass_slot_count);
        slot_count = superclass_slot_count;
    }

    for (i = 0; i < descriptor_slot_count; i++){
        LTE_Value slot_name = (LTE_Value)(uintptr_t)descriptor_slots[i].name;
        LTE_Class_Slot slot = {
            .name = slot_name,
            .offset = descriptor_slots[i].offset,
            .type = descriptor_slots[i].type,
        };
        size_t j;
        int replaced = 0;

        if (slot.type == NULL){
            slot.type = &LTE_SlotType_Object;
        }

        for (j = 0; j < slot_count; j++){
            if (slots[j].name == slot_name){
                slots[j] = slot;
                replaced = 1;
                break;
            }
        }

        if (!replaced){
            slots[slot_count] = slot;
            slot_count++;
        }
    }

    klass->slot_count = slot_count;
    klass->slots = slots;
    if (slot_count > 1){
        qsort(klass->slots,
              klass->slot_count,
              sizeof(LTE_Class_Slot),
              compare_slots_by_name);
    }
}

static void materialize_direct_methods(
    LTE_Class* klass,
    LTE_Method_Descriptor* descriptor_methods
){
    klass->methods = (LTE_Value)(uintptr_t)descriptor_methods;
}

static void finalize_core_class_cycle_if_ready(void){
    if (core_class_cycle_finalized){
        return;
    }

    LTE_Object_class.base.cls = (uintptr_t)&LTE_Object_class_class;
    LTE_Object_class.superclasses = empty_superclasses;
    LTE_Object_class.precedence_list = LTE_NIL;
    LTE_Object_class.instance_size = sizeof(LTE_Object);
    LTE_Object_class.class_flags = LTE_CLASS_FLAG_ALLOCATABLE;
    LTE_Object_class.name = (LTE_Value)(uintptr_t)"Object";
    LTE_Object_class.debugPrint = Class_debugPrint;
    LTE_Object_class.hash = Class_default_hash;
    LTE_Object_class.equal_p = Class_default_equal_p;
    LTE_Object_class.documentation =
        (LTE_Value)(uintptr_t)"Root of the class hierarchy.";

    LTE_Class_class.base.cls = (uintptr_t)&LTE_Class_class_class;
    LTE_Class_class.superclasses = object_superclasses;
    LTE_Class_class.precedence_list = LTE_NIL;
    LTE_Class_class.instance_size = sizeof(LTE_Class);
    LTE_Class_class.class_flags = LTE_CLASS_FLAG_ABSTRACT;
    LTE_Class_class.slot_count = sizeof(Class_slots) / sizeof(Class_slots[0]);
    LTE_Class_class.slots = Class_slots;
    LTE_Class_class.name = (LTE_Value)(uintptr_t)"Class";
    LTE_Class_class.debugPrint = Class_debugPrint;
    LTE_Class_class.hash = Class_default_hash;
    LTE_Class_class.equal_p = Class_default_equal_p;
    LTE_Class_class.documentation =
        (LTE_Value)(uintptr_t)"Runtime representation of classes and metaclasses.";

    LTE_Object_class_class.base.cls = (uintptr_t)&LTE_Class_class_class;
    LTE_Object_class_class.superclasses = class_superclasses;
    LTE_Object_class_class.precedence_list = LTE_NIL;
    LTE_Object_class_class.instance_size = sizeof(LTE_Class);
    LTE_Object_class_class.class_flags = LTE_CLASS_FLAG_ABSTRACT;
    LTE_Object_class_class.slot_count = LTE_Class_class.slot_count;
    LTE_Object_class_class.slots = LTE_Class_class.slots;
    LTE_Object_class_class.name = (LTE_Value)(uintptr_t)"Object class";
    LTE_Object_class_class.debugPrint = Class_debugPrint;
    LTE_Object_class_class.hash = LTE_Class_class.hash;
    LTE_Object_class_class.equal_p = LTE_Class_class.equal_p;

    LTE_Class_class_class.base.cls = (uintptr_t)&LTE_Class_class_class;
    LTE_Class_class_class.superclasses = class_superclasses;
    LTE_Class_class_class.precedence_list = LTE_NIL;
    LTE_Class_class_class.instance_size = sizeof(LTE_Class);
    LTE_Class_class_class.class_flags = LTE_CLASS_FLAG_ABSTRACT;
    LTE_Class_class_class.slot_count = LTE_Class_class.slot_count;
    LTE_Class_class_class.slots = LTE_Class_class.slots;
    LTE_Class_class_class.name = (LTE_Value)(uintptr_t)"Class class";
    LTE_Class_class_class.debugPrint = Class_debugPrint;
    LTE_Class_class_class.hash = LTE_Class_class.hash;
    LTE_Class_class_class.equal_p = LTE_Class_class.equal_p;

    core_class_cycle_finalized = 1;
}

void LTE_init_native_class(LTE_Class* klass){
    LTE_Class_Descriptor* descriptor = klass->native_descriptor;
    LTE_Class* metaclass;

    finalize_core_class_cycle_if_ready();

    if (descriptor == NULL){
        return;
    }

    /* Mark as in-progress before recursive initialization to break cycles. */
    klass->native_descriptor = NULL;

    if (descriptor->superclass != NULL){
        LTE_init_native_class(descriptor->superclass);
    }
    if (descriptor->metaclass_superclass != NULL){
        LTE_init_native_class(descriptor->metaclass_superclass);
    }

    metaclass = (LTE_Class*)LTE_Value_class((LTE_Value)klass);

    klass->instance_size = descriptor->instance_size;
    klass->class_flags = (unsigned int)descriptor->class_flags;
    klass->debugPrint = descriptor->debugPrint;
    klass->finalize = descriptor->finalize;
    klass->mark = descriptor->mark;
    if (descriptor->hash != NULL){
        klass->hash = descriptor->hash;
    } else if (descriptor->superclass != NULL){
        klass->hash = descriptor->superclass->hash;
    } else {
        klass->hash = Class_default_hash;
    }
    if (descriptor->equal_p != NULL){
        klass->equal_p = descriptor->equal_p;
    } else if (descriptor->superclass != NULL){
        klass->equal_p = descriptor->superclass->equal_p;
    } else {
        klass->equal_p = Class_default_equal_p;
    }
    klass->name = descriptor->name == NULL
        ? LTE_NIL
        : (LTE_Value)(uintptr_t)descriptor->name;
    klass->methods = LTE_INVALID;
    klass->method_cache = LTE_INVALID;
    klass->cache_version = 0;
    klass->documentation = descriptor->documentation == NULL
        ? LTE_NIL
        : (LTE_Value)(uintptr_t)descriptor->documentation;
    klass->superclasses = make_single_superclass_list(descriptor->superclass);
    klass->precedence_list = LTE_NIL;
    materialize_slots(klass, descriptor->slots);
    materialize_direct_methods(klass, descriptor->methods);

    if (metaclass != NULL){
        metaclass->base.cls = (uintptr_t)&LTE_Class_class_class;
        if (metaclass->instance_size == 0){
            metaclass->instance_size = sizeof(LTE_Class);
        }
        if (metaclass->debugPrint == NULL){
            metaclass->debugPrint = Class_debugPrint;
        }
        if (descriptor->metaclass_superclass != NULL){
            metaclass->hash = descriptor->metaclass_superclass->hash;
            metaclass->equal_p = descriptor->metaclass_superclass->equal_p;
            metaclass->slot_count = descriptor->metaclass_superclass->slot_count;
            metaclass->slots = descriptor->metaclass_superclass->slots;
        } else {
            metaclass->hash = Class_default_hash;
            metaclass->equal_p = Class_default_equal_p;
            metaclass->slot_count = 0;
            metaclass->slots = NULL;
        }
        metaclass->name = descriptor->name == NULL
            ? LTE_NIL
            : (LTE_Value)(uintptr_t)LTE_sprintf("%s class", descriptor->name);
        metaclass->methods = (LTE_Value)(uintptr_t)descriptor->class_methods;
        metaclass->method_cache = LTE_INVALID;
        metaclass->cache_version = 0;
        metaclass->documentation = LTE_NIL;
        metaclass->superclasses =
            make_single_superclass_list(descriptor->metaclass_superclass);
        metaclass->precedence_list = LTE_NIL;
    }
}

void* LTE_Class_alloc(LTE_Class* klass){
    LTE_Value value = LTE_gc_alloc(klass);

    if (value == LTE_INVALID){
        LTE_error("Out of GC heap");
    }
    return (void*)(uintptr_t)value;
}

void* LTE_Class_alloc_flexible(LTE_Class* klass, size_t flex){
    (void)klass;
    (void)flex;
    LTE_error("Flexible class allocation is not implemented");
}
