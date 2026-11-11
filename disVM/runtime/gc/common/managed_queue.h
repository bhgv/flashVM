/* Copyright (c) 2026 bhgv
 * EN: Private rooted-ticket state shared by native ownership and Fast retirement.
 * RU: Закрытое состояние tickets с корнями для нативного владения и Fast GC.
 */
#ifndef DVM_MANAGED_QUEUE_H
#define DVM_MANAGED_QUEUE_H
#include "../../managed_owner.h"
#include "disvm/guard_registry.h"
#include "channel_sync.h"


/* One retained managed owner/root per ticket; native aliases share its lifetime. */
typedef struct ManagedTicket
{
    unsigned long references;   /* Atomic native owners; no payload access after successful drop. */
    void *value;                /* Immutable managed referent, rooted until Fast retirement. */
    DVM_GuardLease *access;     /* Canonical per-referent sidecar, held until before managed drop. */
    DVM_NativeWorld *world;     /* Immutable identity, borrowed through ticket retirement. */
    unsigned long epoch;        /* Last native-owner retirement cutoff; zero refuses shared retirement. */
    struct ManagedTicket *next; /* Native queue link, written only after last native drop. */
} ManagedTicket;


/* Protected native service metadata; no managed pointer is dereferenced here. */
typedef struct ManagedOwners
{
    DVM_NativeWorld *world;         /* Borrowed through complete ticket/producer retirement. */
    DVM_NativeContext *controller;  /* Startup owner authorized to close from IDLE. */
    DVM_GCWorkers *workers;         /* Borrowed Fast wake target through producer retirement. */
    ManagedTicket *head;            /* Native pending queue, each ticket still owns root/reference. */
    ManagedTicket *tail;            /* Last pending ticket or NULL. */
    unsigned long tickets;          /* All captured owners, including live/pending/draining. */
    unsigned long live;             /* Tickets with at least one native owner. */
    unsigned long pending;          /* Rooted tickets awaiting collector retirement. */
    unsigned long producers;        /* Drop callbacks pin worker lifetime until wake completes. */
    unsigned long captured;         /* Completed capture count for joined diagnostics. */
    unsigned long shared_released;  /* Shared handoffs, protected by the service mutex. */
    unsigned long shared_with_peer; /* Shared handoffs while a guest remains admitted. */
    unsigned long released;         /* Completed managed drops for joined diagnostics. */
    DVM_GuardRegistry *guards;      /* Owned canonical sidecars; every ticket retains one lease. */
    DVM_CellDomain *cells;          /* Allocation-free pointer-cell exclusion for hosted MOVP. */
    int accepting;                  /* Capture admission, independent of native alias retention. */
    int failed;                     /* Wake failure, never permission to remove roots early. */
} ManagedOwners;

extern ManagedOwners dvm_managed_owners;          /* Service metadata protected by its native mutex. */
extern const DVM_OwnerOps dvm_managed_ticket_ops; /* Immutable native alias callbacks. */


/* [GC:] EN: Lock the process-lifetime service mutex; NULL means not initialized.
 * RU: Захватить мьютекс службы до конца процесса; NULL означает отсутствие службы. */
FVM_PortSync *dvm_managed_lock_service(void);


/* [GC:] EN: Abort on native protocol corruption with borrowed message; no VM callback.
 * RU: Завершиться при нарушении нативного протокола с message, без вызова VM. */
void dvm_managed_failure(const char *message);


/* [GC:] EN: Check the current capture role without granting payload ownership.
 * RU: Проверить роль текущего захвата, не предоставляя владения данными. */
int dvm_managed_owner_admitted(void);


/* [GC:] EN: Borrow the sole Fast consumer's head and its world; producers only append.
 * world receives the stable service identity. No root/reference is removed.
 * RU: Получить голову единственного потребителя Fast и мир службы в world.
 * Производители лишь добавляют записи; корень и ссылка остаются во владении. */
ManagedTicket *dvm_managed_retirement_peek(DVM_NativeWorld **world);


/* [GC:] EN: Withdraw exactly ticket and account its completed managed handoff.
 * Caller already consumed its reference/root and sidecar; free native ticket here.
 * RU: Снять именно ticket и учесть завершённую передачу. Вызывающий уже передал
 * ссылку и снял свой корень и sidecar; здесь освободить нативный ticket. */
void dvm_managed_retirement_complete(ManagedTicket *ticket, int peer);
#endif
