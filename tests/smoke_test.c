/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#include <ListTalkEmbedded/ListTalkEmbedded.h>

#include <stdint.h>
#include <string.h>

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
    int value = 42;

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

    LTE_InlineHash_init(&hash);
    LTE_StringHash_at_put(&hash, "answer", &value);
    if (expect(LTE_StringHash_at(&hash, "answer") == &value)){
        return 1;
    }
    LTE_InlineHash_destroy(&hash);

    return 0;
}
