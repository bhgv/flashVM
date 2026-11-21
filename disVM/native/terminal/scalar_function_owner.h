/* Copyright (c) 2026 bhgv
 * EN: Local Unibus routes for a mounted Dis function.
 * RU: Локальные маршруты Unibus для смонтированной функции Dis.
 */
#ifndef STYX_SCALAR_FUNCTION_OWNER_H
#define STYX_SCALAR_FUNCTION_OWNER_H
#include "unibus_inbox.h"
#include <stdint.h>

typedef struct StyxScalarFunctionOwner
{
    DVM_Provider *request; /* Owned mailbox read by the Dis service. */
    DVM_Provider *reply;   /* Owned mailbox written by the service. */
    unsigned long request_tag; /* Compiled-width mailbox tag, fixed at publication. */
    int retired;           /* Dispatcher-only flag after an ambiguous exchange. */
} StyxScalarFunctionOwner;

int styxscalarfunctiondispatch(void *state, int64_t argument, int64_t *value);

#endif
