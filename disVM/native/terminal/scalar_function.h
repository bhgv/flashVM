/* Copyright (c) 2026 bhgv
 * EN: Styx function file with one invocation per open fid.
 * RU: Файл функции Styx с одним вызовом на открытый fid.
 */
#ifndef STYX_SCALAR_FUNCTION_H
#define STYX_SCALAR_FUNCTION_H
#include "scalar_record.h"
#include "tuple_record.h"
#include "call_record.h"
#include "load_profile.h"
#include "ref_pair_record.h"
#include "provider.h"
#include "channel_export.h"

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
    int (*ref_pair_method)(void *, StyxRefPair *); /* Keep the map and update object snapshots in place. */
    const StyxCallShape *dynamic_shape; /* Borrowed validated scalar schema through join. */
    int (*scoped_call_method)(void *, DVM_Provider *, const StyxCallValues *, StyxCallValues *); /* Typed call with a borrowed caller provider. */
    int (*scoped_ref_pair_method)(void *, DVM_Provider *, StyxRefPair *); /* Reference call with a borrowed caller provider. */
    int (*managed_method)(void *, DVM_Provider *, const FVM_SRecord *, unsigned char *, size_t, size_t *); /* Borrow request/context and encode reply. */
} StyxScalarFunction;


/* EN/RU: Check the callback shape and its wire profile. / Проверить callback и профиль. */
int styxscalarfunctionvalid(const StyxScalarFunction *function);

struct Styxserver;
char *styxscalarfunctionopen(struct Styxserver *server, int mode, const StyxScalarFunction *function, int report_transport_failure, int *workers, int *busy);


/* EN: Open one function fid, optionally granting return-channel publication.
 * RU: Открыть fid функции с необязательной выдачей возвращаемых каналов. */
char *styxscalarfunctionopen_export(struct Styxserver *server, int mode, const StyxScalarFunction *function,
    int report_transport_failure, int *workers, int *busy, StyxChannelExports *exports);
char *styxscalarfunctionwrite(void *fid, const char *bytes, ulong count, vlong offset);
void styxscalarfunctionclose(void *fid);
char *styxscalarfunctionread(void *fid, char *bytes, ulong *count, vlong offset);

#endif
