/* Copyright (c) 2026 bhgv
 * EN: Native push subscriptions with refcounted in-flight callback lifetime.
 * RU: Native push subscriptions с refcounted lifetime callbacks в полёте.
 */
#ifndef DVM_VFS_NOTICE_H
#define DVM_VFS_NOTICE_H
#include "file_port.h"
#include <stdint.h>
typedef struct DVM_NoticeHub DVM_NoticeHub; /* Owner-held native source; no managed pointers. */
typedef struct DVM_NoticeTap DVM_NoticeTap; /* Caller-owned subscription handle. */
struct DVM_ChangeEvent;


/* EN/RU: Copy one source event into native recipient state; no VM, I/O, waits or
 * target-thread execution. Called outside hub/journal/namespace locks.
 * Скопировать event в native recipient state; без VM/I/O/waits/участия target. */
typedef void (*DVM_NoticeReceive)(void *state, const struct DVM_ChangeEvent *event);


/* EN/RU: Consume native callback state after unsubscribe and all in-flight calls.
 * Освободить native callback state после unsubscribe и всех in-flight calls. */
typedef void (*DVM_NoticeRelease)(void *state);


/* EN/RU: Allocate owned hub, NULL on allocation/lock failure.
 * Создать свой hub; NULL при отказе allocation/lock. */
DVM_NoticeHub *dvm_notice_create(void);


/* EN/RU: Consume owner ref after all taps were unsubscribed; zero if still attached.
 * In-flight detached taps retain hub until return. Поглотить owner ref после
 * unsubscribe всех taps; ноль при attached taps. In-flight taps удерживают hub. */
int dvm_notice_destroy(DVM_NoticeHub *hub);


/* EN/RU: Subscribe and own state only on success; NULL leaves state caller-owned.
 * Publication is immediate, not a scheduler poll. Retain native recipients in
 * state until release; callbacks already in flight may finish after unsubscribe.
 * Subscribe принимает state только при успехе. Это push, не scheduler poll.
 * Native recipient должен жить до release; in-flight callback может завершиться позже. */
DVM_NoticeTap *dvm_notice_subscribe(DVM_NoticeHub *hub, DVM_NoticeReceive receive,
                                   void *state, DVM_NoticeRelease release);


/* EN/RU: Consume caller subscription; detach without pausing/waiting for target.
 * Поглотить subscription; detach без паузы/ожидания target. One call per handle. */
void dvm_notice_unsubscribe(DVM_NoticeTap *tap);


/* EN/RU: Push one copied source event to current native subscribers immediately.
 * Caller pins hub; no allocation, guest cooperation, scheduler hook or I/O.
 * Передать event subscribers немедленно. Без allocation, guest cooperation и I/O. */
void dvm_notice_publish(DVM_NoticeHub *hub, const struct DVM_ChangeEvent *event);
#endif
