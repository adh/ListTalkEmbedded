/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#ifndef H__ListTalkEmbedded__utils__hex__
#define H__ListTalkEmbedded__utils__hex__

#include <ListTalkEmbedded/macros/env_macros.h>

#include <stddef.h>
#include <stdint.h>

LTE__BEGIN_DECLS

char* LTE_hex_encode(const uint8_t* bytes,
                    size_t length,
                    int uppercase,
                    size_t* length_out);
uint8_t* LTE_hex_decode(const char* string, size_t length, size_t* length_out);

LTE__END_DECLS

#endif
