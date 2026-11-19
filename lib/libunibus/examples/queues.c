/* Copyright (c) 2026 bhgv
 * EN: Hand-reviewed native-thread example: client 7 waits on its own queue while
 * client 8 progresses. Resource cleanup also handles partial initialization.
 * RU: Вручную описанный пример native threads: клиент 7 ждёт свою очередь,
 * клиент 8 получает данные. Ресурсы освобождаются и при частичной инициализации.
 * standalone-reviewed: keep these teaching contracts instead of generating prose.
 */
#include "order.h"   /* Optional native ordered queues; no Styx server or VM is linked. */
#include <pthread.h> /* The consumer is an independent OS thread, not a cooperative task. */
#include <stdlib.h> /* _Exit avoids reclaiming main-stack state if native join unexpectedly fails. */
#include <stdio.h>   /* Stage-specific failure diagnostics are separate from queued payloads. */
#include <string.h>  /* Compare exact byte payloads returned by the receiver engine. */


/* EN: Shared lifetime begins before pthread_create and ends after successful join.
 * RU: Общее время жизни начинается до pthread_create и заканчивается после join.
 */
typedef struct Consumer
{
    StyxOrder *receiver;       /* Borrowed queue engine; main owns and frees it after join. */
    pthread_barrier_t started; /* Main and consumer rendezvous without sleep or polling. */
    int received;             /* Written only by consumer; main reads only after join. */
} Consumer;


/* EN: Wait for client 7 only, then consume its first exact record.
 * argument points to main-owned Consumer, valid until this thread is joined.
 * Returns NULL; received carries success. Wait is bounded to 5000 ms.
 * RU: Ждать только клиента 7, затем поглотить его первую точную запись.
 * argument — Consumer вызывающего, действующий до join этого потока.
 * Вернуть NULL; успех записывается в received. Ожидание ограничено 5000 мс.
 */
static void *consumer(void *argument)
{
    Consumer *context = argument; /* Borrow state whose storage outlives this worker. */
    unsigned char bytes[32];      /* Caller-owned destination; the engine copies one queued record. */
    size_t size;                  /* Published payload length; inspect only after a record result. */
    unsigned long long ordinal;   /* Full per-tag sequence; this tag starts at zero. */
    int barrier;                  /* A serial-thread barrier result is success, not an error. */

    barrier = pthread_barrier_wait(&context->started); /* Rendezvous establishes worker lifetime, not exact wait admission. */
    if (barrier && barrier != PTHREAD_BARRIER_SERIAL_THREAD)
    {
        fprintf(stderr, "consumer barrier wait: %s\n", strerror(barrier)); /* Preserve the worker-side native error. */
        return NULL; /* received remains false; main reports failure after joining. */
    }
    context->received = styxorderwait(context->receiver, 7, 5000) && /* No readiness or wait for client 8 is consulted here. */
        styxordernext(context->receiver, 7, &ordinal, bytes, sizeof(bytes), &size) == STYX_ORDER_RECORD && /* Consume exactly one item. */
        ordinal == 0 && size == 5 && !memcmp(bytes, "seven", 5); /* Sequence and bytes identify client 7's own delivery. */
    return NULL; /* Main synchronizes with this write through pthread_join, not a shared polling flag. */
}


/* EN: Encode and deliver one tag, then consume and check it immediately.
 * sender/receiver are borrowed engines; tag identifies this independent queue.
 * text is a borrowed five-byte payload. Returns nonzero only for exact delivery.
 * No carrier I/O; native queue calls acquire their internal locks.
 * RU: Кодировать, передать и сразу проверить один тег.
 * sender/receiver — заимствованные engines; tag выбирает независимую очередь.
 * text — заимствованный payload из пяти байтов. Неноль только при точной выдаче.
 * Carrier I/O отсутствует; native queue calls используют внутренние lock.
 */
static int deliver_and_read(StyxOrder *sender, StyxOrder *receiver, unsigned long tag, const char *text)
{
    unsigned char packet[512];  /* Encoded ordinal prefix and checked S-record, not a native structure. */
    unsigned char bytes[32];    /* Disjoint receive destination; no borrowed queue pointer escapes. */
    size_t used;                /* Complete packet bytes produced on successful encode. */
    size_t size;                /* Payload bytes copied by next on this tag. */
    unsigned long long ordinal; /* Each independent tag begins at its own ordinal zero. */

    return styxorderencode(sender, tag, text, 5, packet, sizeof(packet), &used) == FVM_S_OK && /* Encode once; uncertain physical sends must retain these bytes. */
        styxorderfeed(receiver, packet, used) == STYX_ORDER_ACCEPTED && /* Service owner dispatches to the selected tag queue. */
        styxordernext(receiver, tag, &ordinal, bytes, sizeof(bytes), &size) == STYX_ORDER_RECORD && /* This nonwaiting next cannot wait for client 7. */
        ordinal == 0 && size == 5 && !memcmp(bytes, text, 5); /* A successful API call alone does not prove the correct client payload. */
}


/* EN: Run without arguments; return 0 on independent client delivery, 1 on a
 * reported setup/delivery/cleanup failure. Own both engines, barrier and worker;
 * close before join on delivery failure to wake a possibly waiting consumer.
 * RU: Запуск без аргументов; вернуть 0 при независимой выдаче, 1 при описанной
 * ошибке настройки/выдачи/cleanup. Владеть engines, barrier и worker;
 * при ошибке выдачи закрыть receiver до join, чтобы разбудить потребителя.
 */
int main(void)
{
    StyxOrder *sender = NULL; /* Sender owns separate TX counters for tags 7 and 8. */
    Consumer context = {0};   /* Zero received also describes a consumer that failed before receiving. */
    pthread_t thread;        /* Used only after pthread_create returns success. */
    unsigned char packet[512]; /* Main's spool remains alive until feed has consumed the packet. */
    size_t used;              /* Valid packet size only after successful encode. */
    int barrier_ready = 0;    /* Destroy only a barrier that was initialized successfully. */
    int worker_started = 0;   /* Never join an uninitialized pthread_t. */
    int good = 0;             /* All success conditions must be observed before exit 0. */
    int error;                /* pthread APIs return error numbers directly, not through errno. */

    sender = styxordercreate(256, 16); /* Local maximum message 256, reordering window 16; no global queue quota. */
    context.receiver = styxordercreate(256, 16); /* Independent native receiver, owned by main. */
    if (!sender || !context.receiver)
    {
        fprintf(stderr, "queue engine allocation failed\n"); /* A null engine must never be passed to the worker. */
        goto cleanup; /* Whichever engine exists is still owned and must be released. */
    }
    error = pthread_barrier_init(&context.started, NULL, 2); /* Exactly main and one consumer participate. */
    if (error)
    {
        fprintf(stderr, "barrier initialization: %s\n", strerror(error)); /* Report the actual pthread result. */
        goto cleanup; /* No worker exists and no initialized barrier needs destruction. */
    }
    barrier_ready = 1; /* From here all exits destroy the barrier after any worker has joined. */
    error = pthread_create(&thread, NULL, consumer, &context); /* State and engine stay alive until join. */
    if (error)
    {
        fprintf(stderr, "consumer creation: %s\n", strerror(error)); /* No attempt is made to wait at a two-party barrier. */
        goto cleanup; /* Destroy the initialized barrier and release both engines. */
    }
    worker_started = 1; /* A later delivery failure must close/wake and join this worker. */
    error = pthread_barrier_wait(&context.started); /* Worker may now call wait; scheduling order remains unspecified. */
    if (error && error != PTHREAD_BARRIER_SERIAL_THREAD)
    {
        fprintf(stderr, "main barrier wait: %s\n", strerror(error)); /* Do not treat SERIAL_THREAD as failure. */
        goto cleanup; /* Wake the receiver before waiting for worker retirement. */
    }
    good = deliver_and_read(sender, context.receiver, 8, "eight"); /* Client 8 must progress before client 7 is delivered. */
    good = good && styxorderencode(sender, 7, "seven", 5, packet, sizeof(packet), &used) == FVM_S_OK && /* Client 7 has an independent first ordinal. */
        styxorderfeed(context.receiver, packet, used) == STYX_ORDER_ACCEPTED; /* Dispatch enqueues without executing consumer code. */
    if (!good)
    {
        fprintf(stderr, "ordered client delivery failed\n"); /* Cleanup closes the receiver, waking an empty wait. */
    }

cleanup:
    styxorderclose(context.receiver); /* Sticky close wakes waits; already-ready records remain drainable. */
    if (worker_started)
    {
        error = pthread_join(thread, NULL); /* The worker can still drain client 7's ready record after close. */
        if (error)
        {
            fprintf(stderr, "consumer join: %s\n", strerror(error)); /* Quiescence was not proven; do not free borrowed state. */
            _Exit(1); /* Terminate without unwinding borrowed stack state whose worker lifetime is unknown. */
        }
    }
    if (barrier_ready)
    {
        error = pthread_barrier_destroy(&context.started); /* No worker can access the barrier after successful join. */
        if (error)
        {
            fprintf(stderr, "barrier destruction: %s\n", strerror(error)); /* Preserve a cleanup failure in the exit status. */
            good = 0; /* Successful delivery does not override failed resource cleanup. */
        }
    }
    styxorderfree(context.receiver); /* No concurrent queue call remains; release ready and held records. */
    styxorderfree(sender); /* No encoded spool references the engine after these sends. */
    if (!good || !context.received)
    {
        if (good)
        {
            fprintf(stderr, "client 7 consumer did not receive its expected record\n"); /* Do not invent a timeout reason from a false result. */
        }
        return 1; /* Diagnostics above or the worker's received flag explain the failing stage. */
    }
    puts("PASS independent dispatcher/client waits, client8 progress and client7 exact ordered delivery"); /* Human stdout report only. */
    return 0; /* Engines, native worker and barrier have all retired. */
}
