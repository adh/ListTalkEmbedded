/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#include <ListTalkEmbedded/utils.h>
#include <ListTalkEmbedded/error.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

uint32_t LTE_fnv_hash(const char* string){
    uint32_t res = 0x811c9dc5;

    while (*string){
        res += (unsigned char)*string;
        res *= 0x01000193;
        string++;
    }
    return res;
}

uint32_t LTE_pointer_hash(const void* pointer){
    uint32_t res = 0x811c9dc5;
    size_t buf = (size_t)pointer;

    buf >>= 3;
    while (buf){
        res += buf & 0xfff;
        buf >>= 12;
        res *= 0x01000193;
    }
    return res;
}

char* LTE_strdup(const char* string){
    size_t len = strlen(string);
    char* res = LTE_malloc(len + 1);

    memcpy(res, string, len);
    res[len] = 0;
    return res;
}

char* LTE_sprintf(const char* fmt, ...){
    va_list args;
    int size;
    char* buf;

    va_start(args, fmt);
    size = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (size < 0){
        return NULL;
    }

    buf = LTE_malloc((size_t)size + 1);

    va_start(args, fmt);
    vsnprintf(buf, (size_t)size + 1, fmt, args);
    va_end(args);

    return buf;
}

char* LTE_strerror(int errnum){
    char buffer[1024];

    if (strerror_r(errnum, buffer, sizeof(buffer)) == 0){
        return LTE_strdup(buffer);
    }
    return LTE_sprintf("Unknown error %d", errnum);
}

void LTE_write_file_bytes_atomically(const char* path,
                                     const void* bytes,
                                     size_t length){
    const unsigned char* byte_cursor = bytes;
    size_t path_length = strlen(path);
    const char* suffix = ".tmp.XXXXXX";
    size_t suffix_length = strlen(suffix);
    char* temp_path = LTE_malloc(path_length + suffix_length + 1);
    int fd;
    size_t offset = 0;

    memcpy(temp_path, path, path_length);
    memcpy(temp_path + path_length, suffix, suffix_length + 1);

    fd = mkstemp(temp_path);
    if (fd < 0){
        LTE_free(temp_path);
        LTE_system_error("Could not create temporary file", errno);
    }

    while (offset < length){
        size_t chunk = length - offset;
        ssize_t written;

        if (chunk > (size_t)SSIZE_MAX){
            chunk = (size_t)SSIZE_MAX;
        }
        written = write(fd, byte_cursor + offset, chunk);
        if (written < 0){
            int saved_errno = errno;

            close(fd);
            unlink(temp_path);
            LTE_free(temp_path);
            LTE_system_error("Could not write file", saved_errno);
        }
        if (written == 0){
            close(fd);
            unlink(temp_path);
            LTE_free(temp_path);
            LTE_error("Could not write file");
        }
        offset += (size_t)written;
    }

    if (close(fd) != 0){
        int saved_errno = errno;

        unlink(temp_path);
        LTE_free(temp_path);
        LTE_system_error("Could not close file", saved_errno);
    }
    if (rename(temp_path, path) != 0){
        int saved_errno = errno;

        unlink(temp_path);
        LTE_free(temp_path);
        LTE_system_error("Could not replace file", saved_errno);
    }

    LTE_free(temp_path);
}

#define STRING_BUILDER_INIT_SIZE 64

struct LTE_StringBuilder {
    size_t length;
    size_t size;
    char* buf;
};

LTE_StringBuilder* LTE_StringBuilder_new(void){
    LTE_StringBuilder* builder = LTE_NEW(LTE_StringBuilder);

    builder->size = STRING_BUILDER_INIT_SIZE;
    builder->buf = LTE_malloc(STRING_BUILDER_INIT_SIZE);
    builder->length = 0;
    builder->buf[builder->length] = 0;
    return builder;
}

void LTE_StringBuilder_free(LTE_StringBuilder* builder){
    if (builder == NULL){
        return;
    }
    LTE_free(builder->buf);
    LTE_free(builder);
}

void LTE_StringBuilder_append_str(LTE_StringBuilder* builder,
                                  const char* string){
    LTE_StringBuilder_append_bytes(builder, string, strlen(string));
}

void LTE_StringBuilder_append_bytes(LTE_StringBuilder* builder,
                                    const char* bytes,
                                    size_t length){
    size_t required_size;

    if (length == 0){
        return;
    }

    required_size = builder->length + length + 1;
    if (builder->size < required_size){
        size_t new_size = builder->size * 2 + 2;

        while (new_size < required_size){
            new_size = new_size * 2 + 2;
        }

        builder->buf = LTE_realloc(builder->buf, new_size);
        builder->size = new_size;
    }

    memcpy(builder->buf + builder->length, bytes, length);
    builder->length += length;
    builder->buf[builder->length] = '\0';
}

void LTE_StringBuilder_append_char(LTE_StringBuilder* builder, char ch){
    LTE_StringBuilder_append_bytes(builder, &ch, 1);
}

char* LTE_StringBuilder_value(LTE_StringBuilder* builder){
    return builder->buf;
}

size_t LTE_StringBuilder_length(LTE_StringBuilder* builder){
    return builder->length;
}

void LTE_InlineHash_init(LTE_InlineHash* h){
    h->mask = 0x7;
    h->count = 0;
    h->vector = LTE_calloc(8, sizeof(LTE_InlineHash_Entry*));
}

void LTE_InlineHash_destroy(LTE_InlineHash* h){
    for (size_t i = 0; i < h->mask + 1; i++){
        LTE_InlineHash_Entry* entry = h->vector[i];

        while (entry != NULL){
            LTE_InlineHash_Entry* next = entry->next;

            if (entry->owns_key){
                LTE_free(entry->key);
            }
            LTE_free(entry);
            entry = next;
        }
    }
    LTE_free(h->vector);
    h->vector = NULL;
    h->mask = 0;
    h->count = 0;
}

size_t LTE_InlineHash_count(LTE_InlineHash* h){
    return h->count;
}

static LTE_InlineHash_Entry* get_string_hash_entry(LTE_InlineHash* h,
                                                   const char* key,
                                                   size_t hash){
    LTE_InlineHash_Entry* i = h->vector[hash & h->mask];

    while (i){
        if (i->hash == hash && strcmp(i->key, key) == 0){
            return i;
        }
        i = i->next;
    }
    return NULL;
}

static LTE_InlineHash_Entry* get_pointer_hash_entry(LTE_InlineHash* h,
                                                    void* key,
                                                    size_t hash){
    LTE_InlineHash_Entry* i = h->vector[hash & h->mask];

    while (i){
        if (i->hash == hash && i->key == key){
            return i;
        }
        i = i->next;
    }
    return NULL;
}

static void grow_table(LTE_InlineHash* h){
    size_t size = (h->mask + 1) << 1;
    LTE_InlineHash_Entry** vector =
        LTE_calloc(size, sizeof(LTE_InlineHash_Entry*));

    for (size_t i = 0; i < h->mask + 1; i++){
        LTE_InlineHash_Entry* entry = h->vector[i];

        while (entry){
            LTE_InlineHash_Entry* next = entry->next;

            entry->next = vector[entry->hash & (size - 1)];
            vector[entry->hash & (size - 1)] = entry;
            entry = next;
        }
    }

    LTE_free(h->vector);
    h->vector = vector;
    h->mask = size - 1;
}

static void add_hash_entry_string(LTE_InlineHash* h,
                                  const char* key,
                                  size_t hash,
                                  void* value){
    LTE_InlineHash_Entry* e;

    if (h->count > h->mask){
        grow_table(h);
    }

    e = LTE_NEW(LTE_InlineHash_Entry);
    h->count++;
    e->key = LTE_strdup(key);
    e->hash = hash;
    e->value = value;
    e->owns_key = 1;
    e->next = h->vector[hash & h->mask];
    h->vector[hash & h->mask] = e;
}

static void add_hash_entry_pointer(LTE_InlineHash* h,
                                   void* key,
                                   size_t hash,
                                   void* value){
    LTE_InlineHash_Entry* e;

    if (h->count > h->mask){
        grow_table(h);
    }

    e = LTE_NEW(LTE_InlineHash_Entry);
    h->count++;
    e->key = key;
    e->hash = hash;
    e->value = value;
    e->owns_key = 0;
    e->next = h->vector[hash & h->mask];
    h->vector[hash & h->mask] = e;
}

void LTE_StringHash_at_put(LTE_InlineHash* h, const char* key, void* value){
    LTE_InlineHash_Entry* e;
    size_t hash = LTE_fnv_hash(key);

    e = get_string_hash_entry(h, key, hash);
    if (e){
        e->value = value;
    } else {
        add_hash_entry_string(h, key, hash, value);
    }
}

void* LTE_StringHash_at(LTE_InlineHash* h, const char* key){
    LTE_InlineHash_Entry* e;
    void* value = NULL;

    e = get_string_hash_entry(h, key, LTE_fnv_hash(key));
    if (e){
        value = e->value;
    }
    return value;
}

int LTE_StringHash_remove(LTE_InlineHash* h,
                          const char* key,
                          void** value_out){
    size_t hash = LTE_fnv_hash(key);
    size_t index;
    LTE_InlineHash_Entry* current;
    LTE_InlineHash_Entry* previous = NULL;

    index = hash & h->mask;
    current = h->vector[index];
    while (current != NULL){
        if (current->hash == hash && strcmp(current->key, key) == 0){
            if (previous == NULL){
                h->vector[index] = current->next;
            } else {
                previous->next = current->next;
            }
            h->count--;
            if (value_out != NULL){
                *value_out = current->value;
            }
            if (current->owns_key){
                LTE_free(current->key);
            }
            LTE_free(current);
            return 1;
        }
        previous = current;
        current = current->next;
    }
    return 0;
}

void LTE_PointerHash_at_put(LTE_InlineHash* h, void* key, void* value){
    LTE_InlineHash_Entry* e;
    size_t hash = LTE_pointer_hash(key);

    e = get_pointer_hash_entry(h, key, hash);
    if (e){
        e->value = value;
    } else {
        add_hash_entry_pointer(h, key, hash, value);
    }
}

void* LTE_PointerHash_at(LTE_InlineHash* h, void* key){
    LTE_InlineHash_Entry* e;
    void* value = NULL;

    e = get_pointer_hash_entry(h, key, LTE_pointer_hash(key));
    if (e){
        value = e->value;
    }
    return value;
}

int LTE_PointerHash_remove(LTE_InlineHash* h,
                           void* key,
                           void** value_out){
    size_t hash = LTE_pointer_hash(key);
    size_t index;
    LTE_InlineHash_Entry* current;
    LTE_InlineHash_Entry* previous = NULL;

    index = hash & h->mask;
    current = h->vector[index];
    while (current != NULL){
        if (current->hash == hash && current->key == key){
            if (previous == NULL){
                h->vector[index] = current->next;
            } else {
                previous->next = current->next;
            }
            h->count--;
            if (value_out != NULL){
                *value_out = current->value;
            }
            LTE_free(current);
            return 1;
        }
        previous = current;
        current = current->next;
    }
    return 0;
}

enum {
    LTE_REGISTERED_CONSTRUCTOR_CAPACITY = 256,
};

static void (*LTE_registered_constructors[LTE_REGISTERED_CONSTRUCTOR_CAPACITY])(void);
static size_t LTE_registered_constructor_count = 0;
static size_t LTE_registered_constructor_run_count = 0;
static int LTE_registered_constructors_running = 0;
static int LTE_registered_constructors_ran = 0;

void LTE_register_constructor(void (*ctor)(void)){
    if (LTE_registered_constructors_ran &&
        !LTE_registered_constructors_running){
        ctor();
        return;
    }

    if (LTE_registered_constructor_count >=
        LTE_REGISTERED_CONSTRUCTOR_CAPACITY){
        fputs("Too many ListTalkEmbedded native constructors\n", stderr);
        abort();
    }

    LTE_registered_constructors[LTE_registered_constructor_count++] = ctor;
}

void LTE_run_registered_constructors(void){
    if (LTE_registered_constructors_ran){
        return;
    }

    LTE_registered_constructors_running = 1;
    while (LTE_registered_constructor_run_count <
           LTE_registered_constructor_count){
        void (*ctor)(void) =
            LTE_registered_constructors[LTE_registered_constructor_run_count++];
        ctor();
    }
    LTE_registered_constructors_running = 0;
    LTE_registered_constructors_ran = 1;
}
