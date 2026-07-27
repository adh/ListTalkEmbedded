/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#ifndef H__ListTalkEmbedded__Primitive__
#define H__ListTalkEmbedded__Primitive__

#include <ListTalkEmbedded/macros/env_macros.h>

#include <ListTalkEmbedded/tail_call.h>
#include <ListTalkEmbedded/value.h>

LTE__BEGIN_DECLS

typedef struct LTE_Primitive_s LTE_Primitive;

typedef LTE_Value(*LTE_Primitive_Func)(
    LTE_Value arguments,
    LTE_Value invocation_context_kind,
    LTE_Value invocation_context_data,
    LTE_TailCallUnwindMarker* tail_call_unwind_marker
);

#define LTE_PRIMITIVE_FLAG_PURE 0x1

struct LTE_Primitive_s {
    LTE_Object base;
    LTE_Primitive_Func function;
    unsigned int flags;
    char* name;
    char* arguments;
    char* description;
};

#define LTE_PRIMITIVE_IMPL_NAME(primitive_object_name) \
    primitive_object_name##_impl

#define LTE_PRIMITIVE_HEAD(primitive_object_name) \
    static LTE_Value LTE_PRIMITIVE_IMPL_NAME(primitive_object_name)( \
        LTE_Value arguments, \
        LTE_Value invocation_context_kind, \
        LTE_Value invocation_context_data, \
        LTE_TailCallUnwindMarker* tail_call_unwind_marker \
    )

#define LTE_DECLARE_PRIMITIVE( \
    primitive_object_name, \
    primitive_name, \
    primitive_arguments, \
    primitive_description \
) \
    LTE_DECLARE_PRIMITIVE_FLAGS( \
        primitive_object_name, \
        primitive_name, \
        primitive_arguments, \
        primitive_description, \
        0 \
    )

#define LTE_DECLARE_PRIMITIVE_FLAGS( \
    primitive_object_name, \
    primitive_name, \
    primitive_arguments, \
    primitive_description, \
    primitive_flags \
) \
    LTE_PRIMITIVE_HEAD(primitive_object_name); \
    static LTE_Primitive primitive_object_name = { \
        .function = LTE_PRIMITIVE_IMPL_NAME(primitive_object_name), \
        .flags = primitive_flags, \
        .name = primitive_name, \
        .arguments = primitive_arguments, \
        .description = primitive_description \
    }

#define LTE_DEFINE_PRIMITIVE( \
    primitive_object_name, \
    primitive_name, \
    primitive_arguments, \
    primitive_description \
) \
    LTE_DECLARE_PRIMITIVE( \
        primitive_object_name, \
        primitive_name, \
        primitive_arguments, \
        primitive_description \
    ); \
    LTE_PRIMITIVE_HEAD(primitive_object_name)

#define LTE_DEFINE_PRIMITIVE_FLAGS( \
    primitive_object_name, \
    primitive_name, \
    primitive_arguments, \
    primitive_description, \
    primitive_flags \
) \
    LTE_DECLARE_PRIMITIVE_FLAGS( \
        primitive_object_name, \
        primitive_name, \
        primitive_arguments, \
        primitive_description, \
        primitive_flags \
    ); \
    LTE_PRIMITIVE_HEAD(primitive_object_name)

LTE_Value LTE_Primitive_new(char* name,
                            char* arguments,
                            char* description,
                            LTE_Primitive_Func function);
LTE_Value LTE_Primitive_new_with_flags(char* name,
                                       char* arguments,
                                       char* description,
                                       LTE_Primitive_Func function,
                                       unsigned int flags);
LTE_Value LTE_Primitive_from_static(LTE_Primitive* primitive);
char* LTE_Primitive_name(LTE_Primitive* primitive);
char* LTE_Primitive_arguments(LTE_Primitive* primitive);
char* LTE_Primitive_description(LTE_Primitive* primitive);
LTE_Primitive_Func LTE_Primitive_function(LTE_Primitive* primitive);
unsigned int LTE_Primitive_flags(LTE_Primitive* primitive);
LTE_Value LTE_Primitive_call(
    LTE_Value primitive,
    LTE_Value arguments,
    LTE_Value invocation_context_kind,
    LTE_Value invocation_context_data,
    LTE_TailCallUnwindMarker* tail_call_unwind_marker
);

LTE__END_DECLS

#endif
