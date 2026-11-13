/* Copyright (c) 2026 bhgv
 * [GC:] EN: Bind native proxy sessions to independently rooted managed exports.
 * RU: Связать native proxy sessions с управляемыми exports и отдельными корнями.
 */
#ifndef DVM_REMOTE_MANAGED_H
#define DVM_REMOTE_MANAGED_H
#include "../remote/session.h"


/* [GC:] EN/RU: Opaque owner-local roots and one thread-safe native admission table.
 * Непрозрачные owner-local roots и одна thread-safe native admission table. */
typedef struct DvmRemoteManaged DvmRemoteManaged;


/* [GC:] EN/RU: Owner-local borrowed view of active invocation's real values.
 * Owner-local borrowed view реальных values active invocation. */
typedef struct
{
    DvmRemoteRequest request;    /* Copied admission metadata, not new execution authority. */
    void *target;                /* Borrowed actual managed owner value. */
    void *refs[DVM_REMOTE_REFS]; /* Borrowed actual values, preserving alias identity. */
} DvmRemoteManagedCall;


/* [GC:] EN: Create on the current ACTIVE registered native owner, which must
 * outlive the adapter. Native allocation only; NULL on wrong role/OOM.
 * RU: Создать на текущем ACTIVE registered owner, живущем дольше adapter.
 * Только native allocation; NULL при неверной роли/OOM. */
DvmRemoteManaged *dvm_remote_managed_new(size_t objects, size_t requests, uint64_t expiry);


/* [GC:] EN: Borrow a valid live managed value already rooted/owned by the caller.
 * On success add a separate strong reference AND registry root, then publish its
 * opaque token. Failure preserves caller/output and rolls back added ownership.
 * Exact creating ACTIVE owner only; no payload read/type coercion or VM invoke.
 * RU: Заимствовать живое managed value, уже rooted/owned caller-ом. При успехе
 * добавить отдельные strong ref И registry root, затем opaque token. Отказ
 * сохраняет caller/output и откатывает добавленное владение. Только creating
 * ACTIVE owner; без чтения payload/type coercion или вызова VM. */
DvmRemoteStatus
dvm_remote_managed_publish(DvmRemoteManaged *adapter, DvmRemoteKind kind, void *value, DvmRemoteHandle *output);


/* EN: Thread-safe native admission/control calls; adapter lifetime is borrowed
 * until caller joins. They never read managed objects or execute effects. Only
 * OK admit authorizes a trusted executor; finish follows effect/result commit.
 * RU: Thread-safe native admission/control; adapter заимствован до join callers.
 * Managed объекты не читаются, effects не выполняются. Только OK admit разрешает
 * trusted исполнение; finish следует после effect/result commit. */
DvmRemoteStatus dvm_remote_managed_admit(DvmRemoteManaged *adapter, const DvmRemoteRequest *request, int *done);
DvmRemoteStatus dvm_remote_managed_finish(DvmRemoteManaged *adapter, uint64_t id);
DvmRemoteStatus dvm_remote_managed_release(DvmRemoteManaged *adapter, DvmRemoteHandle handle);
DvmRemoteStatus dvm_remote_managed_retain(DvmRemoteManaged *adapter, DvmRemoteHandle handle);
DvmRemoteStatus dvm_remote_managed_tick(DvmRemoteManaged *adapter, uint64_t now, uint64_t expiry);
void dvm_remote_managed_close(DvmRemoteManaged *adapter);


/* [GC:] EN: Creating ACTIVE owner resolves the exact active request to local
 * managed values. Executor keeps its call unfinished and never collects these
 * exports until use ends. No invocation, value copy, payload guards or authority
 * to execute again is granted. INVALID/STALE leave output untouched.
 * RU: Creating ACTIVE owner получает local managed values active request.
 * Executor сохраняет call незавершённым и не собирает exports до конца работы.
 * Нет invocation, копирования values, payload guards или права исполнить снова.
 * INVALID/STALE не меняют output; wrong context отвергается до доступа. */
DvmRemoteStatus dvm_remote_managed_resolve(DvmRemoteManaged *adapter, uint64_t id, DvmRemoteManagedCall *output);


/* [GC:] EN: Creating ACTIVE owner moves up to budget eligible roots to concurrent
 * Fast, preserving ownership on capacity. released receives successful transfers.
 * Zero budget/NULL output is INVALID; no managed destructor runs here.
 * RU: Creating ACTIVE owner передаёт до budget готовых roots конкурентному Fast;
 * capacity сохраняет ownership. released получает число передач. Zero budget/NULL
 * output — INVALID; managed destructor здесь не выполняется. */
DvmRemoteStatus dvm_remote_managed_collect(DvmRemoteManaged *adapter, size_t budget, size_t *released);


/* EN: After native callers join, destroy on creating ACTIVE owner only when all
 * roots transferred and admitted calls finished; BUSY preserves the adapter.
 * RU: После join native callers удалить на creating ACTIVE owner только после
 * передачи roots и завершения calls; BUSY сохраняет adapter. */
DvmRemoteStatus dvm_remote_managed_destroy(DvmRemoteManaged *adapter);


/* [GC:] EN/RU: Diagnostic on startup controller after concurrent GC starts.
 * Диагностика startup controller после запуска конкурентного GC. No reference VM. */
void dvm_remote_managed_probe(void);

extern int dvm_remote_managed_probe_requested; /* Immutable CLI selection before owner startup. */
#endif
