/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#ifndef H__ListTalkEmbedded__utils__
#define H__ListTalkEmbedded__utils__

#include <ListTalkEmbedded/alloc.h>
#include <ListTalkEmbedded/macros/env_macros.h>
#include <ListTalkEmbedded/utils/base64.h>
#include <ListTalkEmbedded/utils/crypto/ascon.h>
#include <ListTalkEmbedded/utils/crypto/sha256.h>
#include <ListTalkEmbedded/utils/hex.h>
#include <ListTalkEmbedded/utils/utf8.h>

#include <stdarg.h>
#include <stdint.h>

LTE__BEGIN_DECLS

uint32_t LTE_fnv_hash(const char* string);
uint32_t LTE_pointer_hash(const void* pointer);

char* LTE_strdup(const char* string);
char* LTE_sprintf(const char* fmt, ...);
char* LTE_strerror(int errnum);
void LTE_write_file_bytes_atomically(const char* path,
                                     const void* bytes,
                                     size_t length);

typedef struct LTE_StringBuilder LTE_StringBuilder;

LTE_StringBuilder* LTE_StringBuilder_new(void);
void LTE_StringBuilder_free(LTE_StringBuilder* builder);
void LTE_StringBuilder_append_str(LTE_StringBuilder* builder,
                                  const char* string);
void LTE_StringBuilder_append_bytes(LTE_StringBuilder* builder,
                                    const char* bytes,
                                    size_t length);
void LTE_StringBuilder_append_char(LTE_StringBuilder* builder, char ch);
char* LTE_StringBuilder_value(LTE_StringBuilder* builder);
size_t LTE_StringBuilder_length(LTE_StringBuilder* builder);

typedef struct LTE_InlineHash LTE_InlineHash;
typedef struct LTE_InlineHash_Entry LTE_InlineHash_Entry;

struct LTE_InlineHash {
    LTE_InlineHash_Entry** vector;
    size_t mask;
    size_t count;
};

struct LTE_InlineHash_Entry {
    size_t hash;
    void* key;
    void* value;
    int owns_key;
    LTE_InlineHash_Entry* next;
};

void LTE_InlineHash_init(LTE_InlineHash* h);
void LTE_InlineHash_destroy(LTE_InlineHash* h);
size_t LTE_InlineHash_count(LTE_InlineHash* h);

void LTE_StringHash_at_put(LTE_InlineHash* h, const char* key, void* value);
void* LTE_StringHash_at(LTE_InlineHash* h, const char* key);
int LTE_StringHash_remove(LTE_InlineHash* h,
                          const char* key,
                          void** value_out);

void LTE_PointerHash_at_put(LTE_InlineHash* h, void* key, void* value);
void* LTE_PointerHash_at(LTE_InlineHash* h, void* key);
int LTE_PointerHash_remove(LTE_InlineHash* h,
                           void* key,
                           void** value_out);

void LTE_register_constructor(void (*ctor)(void));
void LTE_run_registered_constructors(void);

#define LTE_REGISTER_CONSTRUCTOR(ctor) \
    void __attribute__((constructor)) LTE___register_constructor_##ctor(void){ \
        LTE_register_constructor(ctor); \
    }

#define LTE_REGISTER_INITIALIZER(init) \
    void __attribute__((constructor)) LTE___register_initializer_##init(void){ \
        LTE_register_constructor(init); \
    }

LTE__END_DECLS

#endif
