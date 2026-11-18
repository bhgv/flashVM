/* Copyright (c) 2026 bhgv
 * EN: Deterministic native TX-capacity admission, cancellation, timeout and no-replay unit checks.
 * RU: Детерминированные native TX-capacity проверки принятия, отмены, timeout и no-replay.
 */
#include "../internal.h"
#include "cancel.h"
#include <limits.h>
#include <stdio.h>
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER; /* Protects subscription checkpoint. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Native checkpoint, no polling. */
static int subscribed; /* Successful cancellation subscription has been published. */


/* EN/RU: Explicit fixture failure in every build profile, diagnostic borrowed.
 * Явная ошибка fixture в любом build profile, diagnostic заимствован.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "TX wait: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Original subscription function, callback/state ownership is unchanged by the checkpoint.
 * Исходная подписка, checkpoint не меняет ownership callback/state.
 */
extern DVM_CancelTap *__real_dvm_cancel_subscribe(DVM_Cancel *, void (*)(void *), void *, void (*)(void *));


/* EN/RU: Signal successful subscription before the sender can block on the control mutex.
 * Сообщить об успешной подписке до блокировки sender на control mutex.
 */
DVM_CancelTap *__wrap_dvm_cancel_subscribe(DVM_Cancel *source, void (*receive)(void *), void *state, void (*release)(void *))
{
    DVM_CancelTap *tap = __real_dvm_cancel_subscribe(source, receive, state, release); /* Original ownership semantics. */
    pthread_mutex_lock(&gate);
    subscribed = tap != NULL;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&gate);
    return tap;
}


/* EN/RU: Private no-I/O control fixture; real PHY/worker behavior is covered by original 9P server tests.
 * Private control fixture без I/O; настоящие PHY/workers проверяются original 9P server tests.
 */
static StyxBidirControl *fixture(void)
{
    StyxBidirControl *control = calloc(1, sizeof(*control)); /* Private native owner, no workers/fids. */
    check(control && !pthread_mutex_init(&control->lock, NULL), "fixture mutex");
    pthread_condattr_t attributes; /* Monotonic condition configuration. */
    check(!pthread_condattr_init(&attributes) && !pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC) &&
        !pthread_cond_init(&control->changed, &attributes), "fixture condition");
    pthread_condattr_destroy(&attributes);
    control->limit = 64;
    control->slots = 1;
    control->extent = 96;
    control->incoming.bytes = malloc(control->extent);
    control->outgoing.bytes = malloc(control->extent);
    control->decode = malloc(control->extent);
    check(control->incoming.bytes && control->outgoing.bytes && control->decode, "bounded fixture storage");
    return control;
}


/* EN/RU: Stack-owned sender state, valid through native thread join.
 * Состояние sender на стеке, живёт до join native thread.
 */
typedef struct Sender
{
    StyxBidirControl *control; /* Borrowed through join. */
    DVM_Cancel *cancel; /* Borrowed source pin retained by main. */
    int result; /* Written by sender, read only after join. */
} Sender;


/* EN/RU: Attempt one record admission under a long native deadline; return result through joined state.
 * Принять одну запись под длинным native deadline, результат через joined state.
 */
static void *sending(void *state)
{
    Sender *sender = state; /* Pinned host stack state. */
    static const unsigned char bytes[] = {0, 128, 255}; /* Independent binary payload. */
    sender->result = styxbidircontrolsendwait(sender->control, 8, bytes, sizeof(bytes), 120000, sender->cancel);
    return NULL;
}


/* EN/RU: Check one exact encoded queue head without consuming it; caller holds control mutex or no workers.
 * Проверить точный encoded head без потребления; caller держит lock либо workers отсутствуют.
 */
static void head(StyxBidirControl *control, unsigned long expected)
{
    FVM_SRecord record; /* Borrowed private queue view. */
    size_t used; /* Exact verified frame extent. */
    check(control->outgoing.count == 1 &&
        fvm_srecord_read(control->outgoing.bytes, control->outgoing.sizes[0], control->limit, &record, &used) == FVM_S_OK &&
        record.tag == expected && used == control->outgoing.sizes[0], "one exact queue head, no overwrite/replay");
    if (expected == 8)
    {
        static const unsigned char bytes[] = {0, 128, 255}; /* Independent expected copy. */
        check(record.size == sizeof(bytes) && !memcmp(record.payload, bytes, sizeof(bytes)), "exact binary queue copy");
    }
}


/* EN/RU: Exercise full/empty queue cancellation and timeout without any transport effect or guest pause.
 * Проверить отмену/timeout full/empty очереди без transport effects и guest pause.
 */
int main(int argc, char **argv)
{
    check(argc == 2, "variant argument");
    StyxBidirControl *control = fixture(); /* Private control owner. */
    DVM_Cancel *source = dvm_cancel_create(); /* Main source pin through all calls. */
    check(source != NULL, "native source");
    if (!strcmp(argv[1], "pre"))
    {
        dvm_cancel_request(source);
        check(!styxbidircontrolsendwait(control, 8, "x", 1, 120000, source) && !control->outgoing.count,
            "sticky cancellation precedes free TX capacity");
    }
    else
    {
        check(styxbidircontrolsend(control, 7, "first", 5), "initial full one-slot queue");
        if (!strcmp(argv[1], "invalid"))
        {
            check(!styxbidircontrolsendwait(control, 8, NULL, 1, 120000, source) &&
                !styxbidircontrolsendwait(control, 8, "x", 1, 120001, source), "invalid inputs before capacity wait");
#if ULONG_MAX > 0xffffffffUL
            check(!styxbidircontrolsendwait(control, 0x100000000UL, "x", 1, 120000, source),
                "invalid tag refuses before capacity wait");
#endif
        }
        else if (!strcmp(argv[1], "timeout"))
        {
            check(!styxbidircontrolsendwait(control, 8, "x", 1, 20, NULL), "full TX deadline expires without mutation");
        }
        else
        {
            Sender sender = {control, source, -1}; /* Independent native admission attempt. */
            pthread_t thread; /* Joined before fixture/source disposal. */
            pthread_mutex_lock(&control->lock);
            check(!pthread_create(&thread, NULL, sending, &sender), "native sender");
            pthread_mutex_lock(&gate);
            while (!subscribed)
            {
                pthread_cond_wait(&changed, &gate);
            }
            pthread_mutex_unlock(&gate);
            head(control, 7);
            if (!strcmp(argv[1], "space"))
            {
                control->outgoing.count = 0;
                pthread_cond_broadcast(&control->changed);
            }
            pthread_mutex_unlock(&control->lock);
            if (!strcmp(argv[1], "cancel"))
            {
                dvm_cancel_request(source);
            }
            check(!pthread_join(thread, NULL) && sender.result == !strcmp(argv[1], "space"),
                "capacity/cancel has exactly one admission outcome");
        }
        head(control, !strcmp(argv[1], "space") ? 8 : 7);
    }
    check(styxbidircontrolstate(control) == STYX_CONTROL_LIVE, "local wait never stops shared control");
    styxbidircontrolfree(control);
    dvm_cancel_request(source);
    dvm_cancel_release(source);
    puts("PASS isolated native TX wait and admission");
    return 0;
}
