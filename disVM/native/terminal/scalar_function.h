/* Copyright (c) 2026 bhgv
 * EN: Styx function file with one invocation per open fid.
 * RU: Файл функции Styx с одним вызовом на открытый fid.
 */
#ifndef STYX_SCALAR_FUNCTION_H
#define STYX_SCALAR_FUNCTION_H
#include "scalar_record.h"

typedef struct StyxScalarFunction
{
    unsigned long request_tag; /* Expected request frame tag. */
    unsigned long reply_tag;   /* Response frame tag. */
    int (*method)(void *, int64_t, int64_t *); /* Borrowed synchronous native method. */
    void *state; /* Borrowed method context. */
    int (*caller_method)(void *, uint32_t, int64_t, int64_t *); /* Opt-in callback with a session-local PID. */
} StyxScalarFunction;


struct Styxserver;
char *styxscalarfunctionopen(struct Styxserver *server, int mode, const StyxScalarFunction *function, int report_transport_failure);
char *styxscalarfunctionwrite(void *fid, const char *bytes, ulong count, vlong offset);
char *styxscalarfunctionread(void *fid, char *bytes, ulong *count, vlong offset);

#endif
