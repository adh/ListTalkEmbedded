/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#ifndef H__ListTalkEmbedded__error__
#define H__ListTalkEmbedded__error__

#include <ListTalkEmbedded/macros/env_macros.h>

LTE__BEGIN_DECLS

_Noreturn void LTE_error_impl(const char* message, ...);
#define LTE_error(...) LTE_error_impl(__VA_ARGS__, NULL)

_Noreturn void LTE_system_error(const char* message, int errnum);

LTE__END_DECLS

#endif
