/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#ifndef H__ListTalkEmbedded__utils__utf8__
#define H__ListTalkEmbedded__utils__utf8__

#include <ListTalkEmbedded/macros/env_macros.h>

#include <stdint.h>
#include <stddef.h>

LTE__BEGIN_DECLS

#define LTE_UTF8_REPLACEMENT_CODEPOINT UINT32_C(0xfffd)

static inline int LTE_utf8_is_continuation(unsigned char ch){
    return (ch & 0xc0) == 0x80;
}

size_t LTE_utf8_sequence_length(unsigned char first);
int LTE_utf8_sequence_valid(const unsigned char* cursor, size_t length);
int LTE_utf8_sequence_valid_bounded(const unsigned char* cursor,
                                   size_t available,
                                   size_t length);
uint32_t LTE_utf8_decode_valid(const unsigned char* cursor, size_t length);
uint32_t LTE_utf8_codepoint_at(const char* cursor);
uint32_t LTE_utf8_codepoint_at_bounded(const char* cursor, size_t available);
const char* LTE_utf8_next(const char* cursor);
const char* LTE_utf8_next_bounded(const char* cursor, size_t available);
size_t LTE_utf8_encode(uint32_t codepoint, char buffer[4]);

LTE__END_DECLS

#endif
