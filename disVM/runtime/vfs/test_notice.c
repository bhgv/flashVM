/* Copyright (c) 2026 bhgv
 * EN: Native push admission, self-detach and concurrent publication/lifetime tests.
 * RU: Native push admission, self-detach и concurrent publication/lifetime tests.
 */
#include "changes.h"
#include "notice.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


/* EN/RU: Stack-owned counters outlive every sender and detached native tap.
 * Stack counters живут дольше всех senders и detached native taps. */
typedef struct Counts
{
    pthread_mutex_t lock; /* Protects callback/destructor counters. */
    unsigned calls; /* Native deliveries, independent from any VM owner execution. */
    unsigned freed; /* Final tap state cleanup count. */
} Counts;


/* EN/RU: Owned callback state; dispatcher retains it until admitted calls finish.
 * Owned callback state; dispatcher удерживает до завершения admitted calls. */
typedef struct Recipient
{
    Counts *counts; /* Borrowed stack counters, valid through sender joins. */
    DVM_NoticeTap *self; /* Non-NULL asks the callback to detach its own subscription. */
} Recipient;


/* EN/RU: Fail explicitly; safe for test senders, does not depend on assert modes.
 * Явный failure, допустимый для senders, без assert modes. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "native notice: %s\n", label);
        abort();
    }
}


/* EN/RU: Count one synchronous native notification, optionally self-detach.
 * event is borrowed; no VM/owner operations. Посчитать notification; optional self-detach. */
static void receive(void *state, const DVM_ChangeEvent *event)
{
    Recipient *recipient = state;
    DVM_NoticeTap *self = recipient->self;
    require(event->generation == 2, "event payload changed");
    pthread_mutex_lock(&recipient->counts->lock);
    recipient->counts->calls++;
    pthread_mutex_unlock(&recipient->counts->lock);
    if (self)
    {
        recipient->self = NULL;
        dvm_notice_unsubscribe(self);
    }
}


/* EN/RU: Consume callback state after the last admitted callback, outside hub locks.
 * Освободить state после last admitted callback, вне hub locks. */
static void release(void *state)
{
    Recipient *recipient = state;
    pthread_mutex_lock(&recipient->counts->lock);
    recipient->counts->freed++;
    pthread_mutex_unlock(&recipient->counts->lock);
    free(recipient);
}


/* EN/RU: Create one owned native callback context and register it on borrowed hub.
 * Создать owned native context, зарегистрировать на borrowed hub; test fails on OOM. */
static DVM_NoticeTap *subscribe(DVM_NoticeHub *hub, Counts *counts, int self)
{
    Recipient *recipient = calloc(1, sizeof(*recipient));
    DVM_NoticeTap *tap;
    require(recipient != NULL, "allocation");
    recipient->counts = counts;
    tap = dvm_notice_subscribe(hub, receive, recipient, release);
    require(tap != NULL, "subscription");
    if (self)
    {
        recipient->self = tap;
    }
    return tap;
}


/* EN/RU: Publish from an independent native sender, never running a recipient guest.
 * Публиковать из native sender без исполнения recipient guest. Returns NULL. */
static void *sender(void *state)
{
    DVM_ChangeEvent event = {1, 2, DVM_CHANGE_CONTENT, 0, 0};
    unsigned index;
    for (index = 0; index < 10000; index++)
    {
        dvm_notice_publish(state, &event);
    }
    return NULL;
}


/* EN/RU: Called by system inbox test before creating VM-independent mailboxes.
 * Verify synchronous delivery, self-detach and concurrent foreign detach cleanup.
 * Проверить synchronous delivery, self-detach и concurrent foreign detach cleanup. */
void test_native_notices(void)
{
    Counts counts = {PTHREAD_MUTEX_INITIALIZER, 0, 0};
    DVM_NoticeHub *hub = dvm_notice_create();
    DVM_ChangeEvent event = {1, 2, DVM_CHANGE_CONTENT, 0, 0};
    pthread_t a, b;
    unsigned index;
    DVM_NoticeTap *anchor;
    require(hub != NULL, "hub creation");
    subscribe(hub, &counts, 1);
    dvm_notice_publish(hub, &event);
    require(counts.calls == 1 && counts.freed == 1, "self detach needs recipient cooperation");
    anchor = subscribe(hub, &counts, 0);
    require(!pthread_create(&a, NULL, sender, hub) && !pthread_create(&b, NULL, sender, hub), "sender creation");
    for (index = 0; index < 2000; index++)
    {
        DVM_NoticeTap *tap = subscribe(hub, &counts, 0);
        dvm_notice_unsubscribe(tap);
    }
    require(!pthread_join(a, NULL) && !pthread_join(b, NULL), "sender join");
    dvm_notice_unsubscribe(anchor);
    require(counts.calls >= 20001 && counts.freed == 2002, "lost delivery or callback state lifetime leak");
    require(dvm_notice_destroy(hub), "hub retirement");
    pthread_mutex_destroy(&counts.lock);
}
