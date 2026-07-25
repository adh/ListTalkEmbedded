/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#ifndef H__ListTalkEmbedded__ListTalkEmbedded__
#define H__ListTalkEmbedded__ListTalkEmbedded__

#include <ListTalkEmbedded/alloc.h>
#include <ListTalkEmbedded/error.h>
#include <ListTalkEmbedded/utils.h>

LTE__BEGIN_DECLS

void LTE_init(void);

#define LTE_INIT() LTE_init()

LTE__END_DECLS

#endif
