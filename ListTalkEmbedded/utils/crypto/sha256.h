/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#ifndef H__ListTalkEmbedded__utils__crypto__sha256__
#define H__ListTalkEmbedded__utils__crypto__sha256__

#include <ListTalkEmbedded/macros/env_macros.h>

#include <stddef.h>
#include <stdint.h>

LTE__BEGIN_DECLS

#define LTE_SHA256_DIGEST_LENGTH 32

typedef struct LTE_SHA256_s {
    uint32_t state[8];
    uint8_t buffer[64];
    size_t buffer_length;
    uint64_t total_length;
} LTE_SHA256;

void LTE_SHA256_init(LTE_SHA256* sha256);
void LTE_SHA256_update(LTE_SHA256* sha256, const uint8_t* bytes, size_t length);
void LTE_SHA256_finish(LTE_SHA256* sha256,
                      uint8_t digest[LTE_SHA256_DIGEST_LENGTH]);
void LTE_SHA256_digest(const uint8_t* bytes,
                      size_t length,
                      uint8_t digest[LTE_SHA256_DIGEST_LENGTH]);

LTE__END_DECLS

#endif
