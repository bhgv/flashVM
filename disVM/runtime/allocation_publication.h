/* Copyright (c) 2026 bhgv
 * EN: Waitable guarded publication for already rooted allocation owners.
 * RU: Ожидаемая защищённая публикация укоренённых владельцев объектов.
 */
#ifndef DVM_ALLOCATION_PUBLICATION_H
#define DVM_ALLOCATION_PUBLICATION_H
#include "heap_hold.h"


/* [GC:] Capture borrowed destination into empty tail under its guard.
 * BUSY releases all guards before polling GC and sleeping. Caller owns the
 * active context and the destination lifetime. Return NULL or a static error.
 * RU: Захватить заимствованное назначение в пустой tail под блокировкой.
 * При BUSY снять блокировки, опросить GC и ждать. Вернуть NULL или ошибку. */
const char *dvm_allocation_hold_destination(void *destination, DVM_HeapHold *tail);


/* [GC:] Borrow destination and observed tail, transfer candidate on success.
 * All holds follow heap_commit_attempt. BUSY preserves holds across GC polls.
 * Return 1 on publication, 0 on stale comparison, -1 on other refusal.
 * RU: Опубликовать candidate в destination при совпадении с tail.
 * При BUSY удержания сохраняются. Вернуть 1 при успехе, 0 при смене, -1 при отказе. */
int dvm_allocation_commit_destination(void *destination, const DVM_HeapHold *tail,
    DVM_HeapHold *candidate, DVM_HeapHold *retired);

#endif
