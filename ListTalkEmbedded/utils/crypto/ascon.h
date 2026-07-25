/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#ifndef H__ListTalkEmbedded__utils__crypto__ascon__
#define H__ListTalkEmbedded__utils__crypto__ascon__

#include <ListTalkEmbedded/macros/env_macros.h>

#include <stddef.h>
#include <stdint.h>

LTE__BEGIN_DECLS

#define LTE_ASCON_XOF128_SEED_LENGTH 32

typedef struct LTE_AsconXOF128_s {
    uint64_t state[5];
    uint8_t buffer[8];
    size_t buffer_offset;
    size_t buffer_length;
    int need_permute_before_block;
} LTE_AsconXOF128;

void LTE_Ascon_permute(uint64_t state[5], unsigned int rounds);
void LTE_AsconXOF128_init(LTE_AsconXOF128* xof,
                         const uint8_t* input,
                         size_t length);
void LTE_AsconXOF128_squeeze(LTE_AsconXOF128* xof,
                            uint8_t* output,
                            size_t length);

LTE__END_DECLS

#endif
