/* Copyright (c) 2026 bhgv
 * EN: Native CLI relay lifecycle, independent stdin producer and per-tag stdout consumers.
 * RU: Native-жизненный цикл CLI-relay, отдельный stdin-производитель и stdout-потребители по тегам.
 */
#ifndef UNIBUS_CLI_SESSION_H
#define UNIBUS_CLI_SESSION_H
#define _POSIX_C_SOURCE 200809L
#include "config.h"
#include "mux/bidir.h"
#include <pthread.h>


/* EN/RU: Connection owners, released after relay workers/subscriptions; no VM state.
 * Owners соединения освобождаются после relay workers/подписок; без VM-состояния.
 */
typedef struct UnibusCliLink
{
    PhyBidir *mux; /* Owned explicit P9MX pump. */
    StyxClient *client; /* Owned attached original Styx client. */
    DVM_Provider *grant; /* Owned captured remote root. */
    StyxOrdered *peer; /* Owned ordered broker, independent of stdio backpressure. */
} UnibusCliLink;


/* EN/RU: State stays alive until all workers join and callback access is fenced off.
 * State живёт до join всех workers и отделения callback-доступа.
 */
typedef struct UnibusCliSession
{
    pthread_mutex_t lock; /* First stop/failure, quota and completed worker counts. */
    pthread_cond_t changed; /* Native main lifecycle condition. */
    pthread_mutex_t output; /* Serialize complete application S-record writes, never transport root0. */
    int wake[2]; /* Native all-worker cancellation pipe, nonblocking writes. */
    int input_wake[2]; /* Separate stdin retirement wake, allowing ready RX to drain. */
    int stopping; /* Explicit quota/error/main shutdown, closes peer outside lock. */
    int error; /* Sticky nonzero process outcome. */
    int retired; /* Transport ended; readers still drain ready per-client records. */
    size_t received; /* Complete application records actually written to stdout. */
    size_t finished; /* Number of per-tag workers that reached end. */
    const UnibusCliConfig *config; /* Immutable borrowed configuration through join. */
    StyxOrdered *peer; /* Borrowed connection owner through worker/subscription retirement. */
} UnibusCliSession;


/* EN/RU: One fixed-tag output worker, no parent/root authority exposed to stdin.
 * Один output-worker своего тега, без выдачи stdin полномочий parent/root.
 */
typedef struct UnibusCliReader
{
    UnibusCliSession *session; /* Borrowed until join. */
    StyxOrderedClient *client; /* Fixed inner-tag capability. */
    unsigned long tag; /* Application-local output tag, including optional0. */
} UnibusCliReader;


/* EN/RU: Borrow immutable config; create connection owners or clean partial setup on0, no dialing.
 * Заимствовать config; создать owners либо очистить partial setup при0, без dial.
 */
int unibuscli_connect(const UnibusCliConfig *config, UnibusCliLink *link);


/* EN/RU: Consume link owners in dependency order, no workers/subscribers may remain.
 * Поглотить link owners в порядке зависимостей после workers/подписок.
 */
void unibuscli_disconnect(UnibusCliLink *link);


/* EN/RU: Borrow live peer/config, run stdio relay until quota/error/retirement;0 success/2 failure/128+signal interruption.
 * Заимствовать peer/config; выполнить stdio-relay до quota/error/retirement:0/2.
 */
int unibuscli_run(StyxOrdered *peer, const UnibusCliConfig *config);


/* EN/RU: Latch stop/error and wake native stdio operations; never holds state mutex during I/O.
 * Зафиксировать stop/error и разбудить native stdio; I/O не выполняется под state mutex.
 */
void unibuscli_stop(UnibusCliSession *session, int error);


/* EN/RU: Borrow native session until join; no guest execution, one copied frame per operation.
 * Заимствовать session до join; без guest-кода, одна копия frame на операцию.
 */
void *unibuscli_input(void *session);


/* EN/RU: Borrow fixed reader until join; only its client queue is consumed.
 * Заимствовать reader до join; потребляется лишь очередь своего клиента.
 */
void *unibuscli_output(void *reader);
#endif
