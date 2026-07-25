/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#ifndef H__ListTalkEmbedded__env_macros__
#define H__ListTalkEmbedded__env_macros__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if __STDC_VERSION__ < 201112L
#error "ListTalkEmbedded requires a C11-compliant compiler"
#endif

#ifdef __cplusplus
#define LTE__BEGIN_DECLS extern "C" {
#define LTE__END_DECLS   }
#else
#define LTE__BEGIN_DECLS
#define LTE__END_DECLS
#endif

#endif
