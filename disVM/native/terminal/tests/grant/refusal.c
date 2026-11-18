/* Copyright (c) 2026 bhgv
 * EN: Unsupported provider and constructor-time cancellation checks never supply a fallback native endpoint.
 * RU: Проверки unsupported provider и constructor-time cancellation не дают fallback native endpoint.
 */
#include "provider.h"
#include "cancel.h"
#include "client.h"
#include <pthread.h>
#include <string.h>
static unsigned calls; /* Caller-owner-only unsupported factory callback count. */
static pthread_t owner; /* Constructor owner selected before cancellation injection. */
static int armed; /* Owner-only one-shot cancellation injection. */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* Protects injected owner state across native RPC callers. */


/* EN/RU: Dummy native metadata increments a sentinel; the Styx-only adapter must not call it.
 * Dummy native metadata увеличивает sentinel; Styx-only adapter не должен её вызывать.
 */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    (void)state;
    (void)path;
    calls++;
    memset(out, 0, sizeof(*out));
    out->directory = 1;
    out->mode = 0777;
    out->identity.low = 1;
    return FVM_OK;
}


/* EN/RU: Dummy unsupported open increments a sentinel without adopting output or doing I/O.
 * Dummy unsupported open увеличивает sentinel, не принимая output и без I/O.
 */
static FVM_Result openfile(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    (void)state;
    (void)path;
    (void)mode;
    (void)out;
    calls++;
    return FVM_ERROR;
}


/* EN/RU: Stateless dummy factory cleanup, no native or managed callback reentry.
 * Cleanup stateless dummy factory без native/managed callback reentry.
 */
static void destroy(void *state)
{
    (void)state;
}
static const DVM_ProviderOps operations = {info, openfile, destroy, DVM_PROVIDER_STABLE_BIND, NULL}; /* Deliberately not a Styx mount. */


/* EN/RU: Owned anchored PATH-capable generic provider/NULL; it must not be silently replaced with a client's root.
 * Свой anchored PATH-capable generic provider/NULL; нельзя подменять root клиента.
 */
DVM_Provider *terminalgrantunsupported(void)
{
    return dvm_provider_create_native(&operations, NULL, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
}


/* EN/RU: Return sentinel count after constructor refusal, no I/O or mutation.
 * Sentinel count после constructor refusal, без I/O/mutation.
 */
unsigned terminalgrantunsupportedcalls(void)
{
    return calls;
}


/* EN/RU: Arm a one-shot cancellation before scoped root clone on the current constructor owner.
 * Включить cancellation до scoped root clone на текущем constructor owner.
 */
void terminalgrantcancelarm(void)
{
    pthread_mutex_lock(&lock);
    owner = pthread_self();
    armed = 1;
    pthread_mutex_unlock(&lock);
}


/* EN/RU: Original cancellable walk; inputs borrowed through return, exact native acknowledgement result.
 * Исходный cancellable walk; inputs borrowed до return, exact native acknowledgement result.
 */
extern StyxFid *__real_styxclientwalkfromcancel(StyxFid *, const char *, StyxCancel *, StyxTransfer *);


/* EN/RU: Request current source before the first scoped clone, then use the real token/engine to refuse it.
 * Запросить current source до первого scoped clone, затем настоящий token/engine откажет ему.
 */
StyxFid *__wrap_styxclientwalkfromcancel(StyxFid *base, const char *path, StyxCancel *cancel, StyxTransfer *result)
{
    pthread_mutex_lock(&lock);
    int cancel_current = armed && pthread_equal(pthread_self(), owner); /* Only the selected constructor owner is affected. */
    if (cancel_current)
    {
        armed = 0;
    }
    pthread_mutex_unlock(&lock);
    if (cancel_current)
    {
        dvm_cancel_request(dvm_cancel_current());
    }
    return __real_styxclientwalkfromcancel(base, path, cancel, result);
}
