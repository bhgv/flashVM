/* Copyright (c) 2026 bhgv
 * EN: Scoped resource service is a real VFS backend; ambient service attempts are forbidden sentinels.
 * RU: Scoped resource service — настоящий VFS backend; ambient service attempts — запрещённые sentinels.
 */
#include "file_provider.h"
#include <stddef.h>
static unsigned ambient; /* Atomic attempts to open the unrelated attached-root movie service. */


/* EN/RU: Owned actual namespace-backed movie resource service/NULL from shared native fixture.
 * Свой actual namespace-backed movie resource service/NULL общего native fixture.
 */
extern DVM_Provider *exchangeprovider(void);


/* EN/RU: Ambient generation names grant no authority; reject every attempted open, count the probe atomically.
 * Ambient generation names не выдают authority; отказать любому open, атомарно считать probe.
 */
StyxBacking *offerbacking(unsigned generation)
{
    (void)generation;
    __atomic_add_fetch(&ambient, 1, __ATOMIC_RELEASE);
    return NULL;
}


/* EN/RU: Return owned native backing for the explicitly granted scoped service file/NULL.
 * Свой native backing явно выданного scoped service file/NULL.
 */
StyxBacking *terminalgrantbacking(void)
{
    DVM_Provider *service = exchangeprovider(); /* Actual prepared VFS service, not fixture reply fabrication. */
    StyxBacking *backing = service ? styxbacking(service) : NULL; /* Independent retained service on success. */
    dvm_provider_release(service);
    return backing;
}


/* EN/RU: Atomic sentinel observation after fetch completion, no I/O or capability transfer.
 * Atomic sentinel observation после fetch, без I/O/передачи capability.
 */
unsigned terminalgrantambientresources(void)
{
    return __atomic_load_n(&ambient, __ATOMIC_ACQUIRE);
}
