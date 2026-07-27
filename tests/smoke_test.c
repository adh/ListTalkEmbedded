/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#include <ListTalkEmbedded/ListTalkEmbedded.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct TestObject_s {
    LTE_Object base;
    LTE_Value child;
} TestObject;

LTE_DECLARE_CLASS(TestObject)

static void mark_test_object(LTE_Value value){
    TestObject* object = (TestObject*)value;

    LTE_gc_mark_object(object->child);
}

static LTE_Class TestClass = {
    .instance_size = sizeof(TestObject),
    .mark = mark_test_object,
};

static LTE_Slot_Descriptor NativeTest_slots[] = {
    {"child", offsetof(TestObject, child), &LTE_SlotType_Object},
    LTE_NULL_NATIVE_CLASS_SLOT_DESCRIPTOR
};

LTE_DEFINE_CLASS(TestObject) {
    .superclass = &LTE_Object_class,
    .metaclass_superclass = &LTE_Class_class,
    .name = "TestObject",
    .documentation = "Smoke test object class.",
    .instance_size = sizeof(TestObject),
    .class_flags = LTE_CLASS_FLAG_ALLOCATABLE,
    .mark = mark_test_object,
    .slots = NativeTest_slots,
};

static size_t finalized_class_count = 0;

static void finalize_class_object(LTE_Value value){
    (void)value;
    finalized_class_count++;
}

static LTE_Class ClassClass = {
    .instance_size = sizeof(LTE_Class),
    .finalize = finalize_class_object,
};

LTE_DEFINE_PRIMITIVE_FLAGS(
    primitive_test_noop,
    "test-noop",
    "()",
    "Test helper primitive.",
    LTE_PRIMITIVE_FLAG_PURE
){
    (void)arguments;
    (void)invocation_context_kind;
    (void)invocation_context_data;
    (void)tail_call_unwind_marker;
    return LTE_NIL;
}

static int expect(int condition){
    return condition ? 0 : 1;
}

int main(void){
    uint8_t digest[LTE_SHA256_DIGEST_LENGTH];
    const uint8_t bytes[] = {'a', 'b', 'c'};
    size_t length;
    char* hex;
    char* base64;
    LTE_StringBuilder* builder;
    LTE_InlineHash hash;
    LTE_Primitive* primitive = &primitive_test_noop;
    int value = 42;
    uintptr_t* heap;
    LTE_Value parent;
    LTE_Value child;
    LTE_Class* heap_class;
    LTE_Value heap_class_instance;
    TestObject* native_object;

    LTE_INIT();

    LTE_SHA256_digest(bytes, sizeof(bytes), digest);
    hex = LTE_hex_encode(digest, sizeof(digest), 0, &length);
    if (expect(length == 64) ||
        expect(strcmp(hex,
                      "ba7816bf8f01cfea414140de5dae2223"
                      "b00361a396177a9cb410ff61f20015ad") == 0)){
        return 1;
    }
    LTE_free(hex);

    base64 = LTE_base64_encode(bytes, sizeof(bytes), 0, 1, &length);
    if (expect(length == 4) || expect(strcmp(base64, "YWJj") == 0)){
        return 1;
    }
    LTE_free(base64);

    builder = LTE_StringBuilder_new();
    LTE_StringBuilder_append_str(builder, "ab");
    LTE_StringBuilder_append_char(builder, 'c');
    if (expect(strcmp(LTE_StringBuilder_value(builder), "abc") == 0)){
        return 1;
    }
    LTE_StringBuilder_free(builder);

    if (expect(primitive->function != NULL) ||
        expect(primitive->flags == LTE_PRIMITIVE_FLAG_PURE) ||
        expect(strcmp(primitive->name, "test-noop") == 0)){
        return 1;
    }

    LTE_init_native_class(&TestObject_class);
    if (expect(TestObject_class.native_descriptor == NULL) ||
        expect(TestObject_class.superclasses[0] == &LTE_Object_class) ||
        expect(TestObject_class.slot_count == 1) ||
        expect(TestObject_class.slots[0].offset == offsetof(TestObject, child))){
        return 1;
    }

    LTE_InlineHash_init(&hash);
    LTE_StringHash_at_put(&hash, "answer", &value);
    if (expect(LTE_StringHash_at(&hash, "answer") == &value)){
        return 1;
    }
    LTE_InlineHash_destroy(&hash);

    heap = LTE_calloc(256, sizeof(uintptr_t));
    LTE_gc_init(heap, 256 * sizeof(uintptr_t));
    native_object = LTE_Class_ALLOC(TestObject);
    native_object->child = LTE_NIL;
    if (expect(TestObject_p((LTE_Value)native_object))){
        return 1;
    }
    parent = LTE_gc_alloc(&TestClass);
    child = LTE_gc_alloc(&TestClass);
    ((TestObject*)parent)->child = child;
    LTE_gc_register_root(&parent);
    LTE_gc_collect();
    if (expect(parent != LTE_INVALID) || expect(child != LTE_INVALID)){
        return 1;
    }
    LTE_gc_unregister_root(&parent);

    heap_class = (LTE_Class*)LTE_gc_alloc(&ClassClass);
    heap_class->instance_size = sizeof(TestObject);
    heap_class->mark = mark_test_object;
    heap_class_instance = LTE_gc_alloc(heap_class);
    LTE_gc_register_root(&heap_class_instance);
    LTE_gc_collect();
    if (expect(finalized_class_count == 0)){
        return 1;
    }
    LTE_gc_unregister_root(&heap_class_instance);
    LTE_free(heap);

    return 0;
}
