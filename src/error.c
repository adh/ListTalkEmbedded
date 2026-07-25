/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2023 - 2026 Ales Hakl
 */

#include <ListTalkEmbedded/error.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void LTE_error_impl(const char* message, ...){
    va_list args;
    const char* key;

    fputs("ListTalkEmbedded error: ", stderr);
    fputs(message, stderr);

    va_start(args, message);
    while ((key = va_arg(args, const char*)) != NULL){
        fputs("; ", stderr);
        fputs(key, stderr);
    }
    va_end(args);

    fputc('\n', stderr);
    abort();
}

void LTE_system_error(const char* message, int errnum){
    fprintf(stderr,
            "ListTalkEmbedded error: %s: %s\n",
            message,
            strerror(errnum));
    abort();
}
