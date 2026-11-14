/* Copyright (c) 2026 bhgv
 * EN: Bounded scalar diagnostic requests, isolated from ordinary owner messages.
 * RU: Ограниченные scalar diagnostic requests отдельно от обычных owner messages.
 */
#ifndef DVM_DIAGNOSTIC_MAILBOX_H
#define DVM_DIAGNOSTIC_MAILBOX_H
#include <stdint.h>

enum
{
    DVM_DIAG_READY = 1,     /* Accepted request or consumed complete reply. */
    DVM_DIAG_PENDING = 0,   /* Request awaits its permanent owner. */
    DVM_DIAG_INVALID = -1,  /* Invalid scalar argument or output. */
    DVM_DIAG_FULL = -2,     /* All eight request/reply credits are occupied. */
    DVM_DIAG_CLOSED = -3,   /* Owner retired; no new work accepted. */
    DVM_DIAG_MISSING = -4,  /* Target/ticket is no longer registered. */
    DVM_DIAG_DENIED = -5,   /* Ticket belongs to another requester. */
    DVM_DIAG_EXHAUSTED = -6 /* Lifetime ticket counter reached signed-big max. */
};


typedef struct DVM_DiagnosticMailbox DVM_DiagnosticMailbox;


/* EN: Native copied reply; no pointers, guest callbacks or managed roots.
 * RU: Native copied reply; без pointers, guest callbacks либо managed roots. */
typedef struct DVM_DiagnosticSample
{
    int owner_pid;            /* Permanent owner that serviced this request. */
    int64_t cookie;           /* Exact requester scalar, copied only on Ready. */
    unsigned long dispatches; /* Owner-published progress counters. */
    unsigned long turns;      /* Completed guest turns, sampled before dispatches. */
    unsigned long waits;      /* Readiness waits, not syscall count or elapsed time. */
} DVM_DiagnosticSample;


/* EN/RU: Create an empty eight-credit native mailbox; NULL on allocation failure.
 * Создать native mailbox на восемь credits; NULL при отказе allocation. */
DVM_DiagnosticMailbox *dvm_diagnostic_create(void);


/* EN/RU: Destroy only after owner join and all callers finish; release all slots.
 * Удалить после owner join и выхода callers; освободить все slots. */
void dvm_diagnostic_destroy(DVM_DiagnosticMailbox *box);


/* EN: Borrow live box; accept requester/cookie and return a new ticket or zero.
 * No allocation/callback; pending and completed replies share the same quota.
 * RU: Заимствовать live box; принять requester/cookie и вернуть ticket либо ноль.
 * Без allocation/callback; pending и ready replies используют общую quota. */
int dvm_diagnostic_request(DVM_DiagnosticMailbox *box, int requester, int64_t cookie, unsigned long *ticket);


/* EN: Borrow box/output; Pending leaves credit live, Ready/Closed consume it.
 * Other requesters are denied. Every non-Ready output is zero. No target access.
 * RU: Заимствовать box/output; Pending хранит credit, Ready/Closed поглощают его.
 * Чужой requester запрещён. Любой non-Ready output нулевой. Без target access. */
int dvm_diagnostic_reply(DVM_DiagnosticMailbox *box, int requester, unsigned long ticket, DVM_DiagnosticSample *sample);


/* EN/RU: Forget own ticket, releasing its credit; never cancel an ordinary guest operation.
 * Забыть свой ticket и освободить credit; обычная guest operation не отменяется. */
int dvm_diagnostic_forget(DVM_DiagnosticMailbox *box, int requester, unsigned long ticket);


/* EN/RU: Release all credits of a retiring requester; caller keeps box alive.
 * Освободить credits завершающегося requester; caller удерживает box. */
void dvm_diagnostic_forget_requester(DVM_DiagnosticMailbox *box, int requester);


/* EN/RU: Mark pending requests Closed; existing Ready replies remain consumable.
 * Пометить pending requests Closed; прежние Ready replies доступны для чтения. */
void dvm_diagnostic_close(DVM_DiagnosticMailbox *box);


/* EN/RU: Test copied pending state; live box must outlast the call.
 * Проверить pending state; live box должен жить до возврата. */
int dvm_diagnostic_pending(DVM_DiagnosticMailbox *box);


/* EN: Only the permanent owner supplies a private scalar sample; service at most
 * budget (1..2) oldest pending tickets. No callbacks under the mailbox mutex.
 * RU: Только permanent owner передаёт private scalar sample; обработать не более
 * budget (1..2) старейших pending tickets. Без callbacks под mailbox mutex. */
void dvm_diagnostic_answer(DVM_DiagnosticMailbox *box, const DVM_DiagnosticSample *sample, unsigned int budget);


/* EN: Atomic hint to skip registry cleanup when no diagnostic credits exist.
 * A retiring single-owner requester has finished all its submissions first.
 * RU: Atomic hint для пропуска registry cleanup при отсутствии credits.
 * Завершающийся single-owner requester уже закончил свои submissions. */
int dvm_diagnostic_has_requests(void);


/* EN/RU: Print atomic lifecycle accounting after all resident owners join.
 * Вывести atomic lifecycle accounting после join всех resident owners. */
void dvm_diagnostic_report(void);
#endif
