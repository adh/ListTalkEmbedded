/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#ifndef H__ListTalkEmbedded__utils__base64__
#define H__ListTalkEmbedded__utils__base64__

#include <ListTalkEmbedded/macros/env_macros.h>

#include <stddef.h>
#include <stdint.h>

LTE__BEGIN_DECLS

char* LTE_base64_encode(const uint8_t* bytes,
                       size_t length,
                       int uri_safe,
                       int include_padding,
                       size_t* length_out);
uint8_t* LTE_base64_decode(const char* string,
                          size_t length,
                          int uri_safe,
                          int require_padding,
                          size_t* length_out);

LTE__END_DECLS

#endif
