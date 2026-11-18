/* Copyright (c) 2026 bhgv
 * EN: Prepared native VFS resource service exported as ordinary independent per-fid Styx streams.
 * RU: Prepared native VFS resource service как обычные независимые per-fid Styx streams.
 */
#include "service.h"
#include "file_provider.h"


/* EN/RU: Owned actual fixture service over a confined native provider, no VM or socket construction.
 * Owned настоящий fixture service над native provider, без VM/socket construction.
 */
extern DVM_Provider *exchangeprovider(void);


/* EN/RU: Return owned ordinary backing for one scoped generation/NULL; each open remains independent.
 * Вернуть owned backing для scoped generation/NULL, каждый open остаётся независимым.
 */
StyxBacking *offerbacking(unsigned generation)
{
    if (generation < 1 || generation > 3)
    {
        return NULL;
    }
    DVM_Provider *service = exchangeprovider(); /* Owned actual VFS service. */
    StyxBacking *backing = service ? styxbacking(service) : NULL; /* Independent service pin on success. */
    dvm_provider_release(service);
    return backing;
}
