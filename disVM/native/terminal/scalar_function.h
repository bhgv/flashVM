/* Copyright (c) 2026 bhgv
 * EN: Styx function file with one invocation per open fid.
 * RU: Файл функции Styx с одним вызовом на открытый fid.
 */
#ifndef STYX_SCALAR_FUNCTION_H
#define STYX_SCALAR_FUNCTION_H
#include "scalar_record.h"
#include "tuple_record.h"
#include "call_record.h"
#include "provider.h"

typedef struct StyxScalarFunction
{
    unsigned long request_tag; /* Expected request frame tag. */
    unsigned long reply_tag;   /* Response frame tag. */
    int (*method)(void *, int64_t, int64_t *); /* Borrowed synchronous native method. */
    void *state; /* Borrowed method context. */
    int (*caller_method)(void *, uint32_t, int64_t, int64_t *); /* Opt-in callback with a session-local PID. */
    DVM_Provider *program_root; /* Borrowed reverse module root through server join. */
    int (*program_method)(void *, DVM_Provider *, int64_t, int64_t *); /* Borrow the exact .self during dispatch. */
    int (*tuple_method)(void *, DVM_Provider *, const StyxTuple *, StyxTuple *); /* One typed tuple call. */
    int (*ref_method)(void *, DVM_Provider *, const StyxTuple *, StyxRefReply *); /* Return and copy-back fields. */
    int (*call_method)(void *, const StyxCallValues *, StyxCallValues *); /* Typed argument-list callback. */
} StyxScalarFunction;


/* EN/RU: Check the callback shape and its wire profile. / Проверить callback и профиль. */
int styxscalarfunctionvalid(const StyxScalarFunction *function);

struct Styxserver;
char *styxscalarfunctionopen(struct Styxserver *server, int mode, const StyxScalarFunction *function, int report_transport_failure);
char *styxscalarfunctionwrite(void *fid, const char *bytes, ulong count, vlong offset);
char *styxscalarfunctionread(void *fid, char *bytes, ulong *count, vlong offset);

#endif
