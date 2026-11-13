/* Copyright (c) 2026 bhgv
 * EN: Private permanent-owner binding for the guest proxy native module.
 * RU: Приватная привязка guest proxy native module к постоянному owner.
 */
#ifndef DVM_PROXYHOST_H
#define DVM_PROXYHOST_H
#include "remote_managed.h"
#include <stdint.h>


/* [GC:] EN/RU: TLS identity and state never migrate to another Prog/thread.
 * TLS identity/state никогда не мигрируют на другой Prog/thread. */
typedef struct
{
    DvmRemoteManaged *adapter; /* Owner-local rooted export adapter, NULL when closed. */
    uint64_t identity;         /* Native session identity, learned on first export. */
    uint64_t active;           /* One currently executing owner request, zero otherwise. */
    size_t objects;            /* Configured root bound and collection slice. */
} DvmProxyHostState;

extern __thread DvmProxyHostState dvm_proxyhost_state; /* Permanent owner TLS through retirement. */


/* EN/RU: Register generated module exactly once before guests start.
 * Зарегистрировать generated module один раз до старта гостей. */
void dvm_proxyhost_initialize(void);


/* [GC:] EN: After owner stack publication and before retirement, finish abandoned
 * call and transfer all export roots. Capacity retry leaves this owner only.
 * RU: После публикации owner stack и до retirement завершить abandoned call
 * и передать export roots. При capacity retry выходит только этот owner. */
void dvm_proxyhost_retire(void);


/* EN/RU: Print joined lifecycle counts if any session opened; no parameters/ownership.
 * Напечатать joined lifecycle counts при наличии sessions; без ownership. */
void dvm_proxyhost_statistics(void);


/* EN/RU: Account one admitted/completed native call using signed delta.
 * Учесть admitted/completed native call по signed delta. */
void dvm_proxyhost_active(int delta);
#endif
