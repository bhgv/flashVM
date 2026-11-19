/* Copyright (c) 2026 bhgv
 * EN: Private gateway contracts: one RX owner, per-client FIFOs/workers and a
 * dedicated wake pipe for terminal output failure. No Styx or VM dependency.
 * RU: Частные контракты: один RX owner, отдельные FIFO/workers клиентов и
 * wake pipe для отказа вывода. Без зависимости от Styx или VM.
 * standalone-reviewed: retain authored contracts when refreshing source metadata.
 */
#ifndef EXAMPLE_BRIDGE_H
#define EXAMPLE_BRIDGE_H
#include <pthread.h>
#include <stddef.h>

#define BRIDGE_LIMIT 8192U /* Maximum opaque 9P frame, including its seven-byte header. */
#define BRIDGE_CLIENTS 2U  /* This teaching profile assigns outer tags 7 and 8 only. */


/* EN/RU: A queue-owned copy becomes worker-owned when removed from the FIFO.
 * Копия принадлежит очереди, затем worker после снятия из FIFO.
 */
typedef struct Packet
{
    struct Packet *next;   /* Owned remaining queue; NULL terminates it. */
    size_t size;           /* Copied frame bytes, zero for an invalid-client marker. */
    int invalid;           /* Report an application error without touching the backend. */
    unsigned char bytes[]; /* Opaque original 9P bytes; never native structures. */
} Packet;


/* EN/RU: Main owns storage until join; socket number remains pinned through join.
 * Main владеет памятью до join; номер socket удерживается до join.
 */
typedef struct Worker
{
    pthread_mutex_t lock; /* Protect queue and flags; never held during socket/output I/O. */
    pthread_cond_t ready; /* Wait for only this client's queued records or ending flag. */
    pthread_t thread;     /* Initialized only after successful bridge_worker_start. */
    Packet *head;         /* Owned oldest pending request. */
    Packet *tail;         /* Borrowed last request, or NULL with an empty queue. */
    int ending;           /* Stop accepting requests; drain admitted ones unless aborting. */
    int aborting;         /* Transport output is lost: no further backend work may start. */
    int failed;           /* Sticky backend failure; later requests receive private errors. */
    int output_failed;    /* Sticky whole-carrier output failure, inspected after join. */
    int socket;           /* Connected backend fd; only main closes it, after join. */
    int notify;           /* Borrowed nonblocking wake-pipe write end, retained through join. */
    unsigned long tag;    /* Immutable outer tag 7 or 8; inner IDs belong to that client. */
} Worker;


/* EN: Connect a fresh owned loopback socket to port (1..65535); return fd/-1.
 * Native connect/I/O timeouts are 3 seconds per syscall, not an end-to-end deadline.
 * RU: Подключить новый owned loopback socket к port (1..65535); вернуть fd/-1.
 * Native timeout 3 с на syscall, не общий deadline запроса.
 */
int bridge_connect(unsigned port);


/* EN: Read/write exactly size bytes on borrowed fd and bytes; writing selects direction.
 * Return 1/0. Continue only the remaining span after partial I/O/EINTR; never replay a request.
 * RU: Читать/писать size байт через fd/bytes; writing выбирает направление.
 * Вернуть 1/0. После partial I/O/EINTR продолжать остаток, не повторять запрос.
 */
int bridge_exact(int fd, void *bytes, size_t size, int writing);


/* EN: Emit one complete outer record from borrowed payload, size bytes, under tag.
 * Return 1/0; serialize physical stdout writes only. A failed write latches this direction terminal. Physical backpressure may block.
 * RU: Выдать целую внешнюю запись payload/size с tag. Вернуть 1/0;
 * сериализуется только stdout; ошибка записи делает направление terminal. Обратное давление может блокировать.
 */
int bridge_record(unsigned long tag, const void *payload, size_t size);


/* EN: Wrap borrowed bytes/size in inner 1 (reply) or 2 (error), then outer tag.
 * Return 1/0; no client gets access to transport tag zero. May block on physical output.
 * RU: Обернуть bytes/size внутренним 1 (ответ) либо 2 (ошибка), затем внешним tag.
 * Вернуть 1/0; клиент не получает transport tag 0. Возможна блокировка вывода.
 */
int bridge_reply(unsigned long tag, unsigned long inner, const void *bytes, size_t size);


/* EN: Decode a little-endian length from at least four borrowed bytes; no bounds check here.
 * RU: Декодировать little-endian длину из минимум четырёх bytes; границу проверяет caller.
 */
unsigned bridge_length(const unsigned char *bytes);


/* EN: Copy borrowed bytes/size into worker's queue; invalid queues an error marker instead.
 * Return 1 on ownership transfer, 0 on allocation/argument/ending failure. Does not wait for backend/output.
 * RU: Скопировать bytes/size в очередь worker; invalid ставит маркер ошибки.
 * Вернуть 1 при передаче владения, 0 при allocation/argument/ending failure. Без ожидания backend/output.
 */
int bridge_enqueue(Worker *worker, const void *bytes, size_t size, int invalid);


/* EN: Thread entry; state borrows Worker until join. Return NULL after drain/abort.
 * Only this worker consumes its queue and performs its backend RPCs, one at a time.
 * RU: Thread entry; state заимствует Worker до join. Вернуть NULL после drain/abort.
 * Только этот worker читает свою очередь и выполняет свои backend RPC по одному.
 */
void *bridge_worker(void *state);


/* EN: Initialize borrowed worker and create its thread. Consume fd on every outcome.
 * tag/notify remain fixed until join; notify must be a live nonblocking pipe writer.
 * Return 1 if running, 0 after releasing every partially initialized resource.
 * RU: Инициализировать worker и создать поток. Поглотить fd при любом исходе.
 * tag/notify неизменны до join; notify — живой nonblocking pipe writer.
 * Вернуть 1 при запуске, 0 после освобождения всех частично созданных ресурсов.
 */
int bridge_worker_start(Worker *worker, int fd, unsigned long tag, int notify);


/* EN: Stop admission to borrowed running worker. aborting also interrupts its socket
 * and forbids new backend effects; otherwise drain accepted requests. No close/free.
 * RU: Прекратить admission worker. aborting прерывает socket и запрещает новые
 * backend effects; иначе дочитать принятые запросы. Без close/free.
 */
void bridge_worker_stop(Worker *worker, int aborting);


/* EN: Join running worker and consume its socket/FIFO/synchronization resources.
 * Return 1 if output and cleanup succeeded, 0 on either failure. Failed join terminates the
 * example process without reclaiming storage whose thread lifetime is unknown.
 * RU: Join worker, поглотить socket/FIFO/синхронизацию. Вернуть 1 при исправном
 * выводе/cleanup, 0 при отказе. При failed join завершить процесс без освобождения
 * памяти, которой ещё может пользоваться поток.
 */
int bridge_worker_finish(Worker *worker);


/* EN: Receive/dispatch from stdin using worker-owned copies; watch borrowed wake fd
 * alongside stdin. Return 1 for complete EOF, 0 for input/protocol failure, -1 for output wake.
 * RU: Принимать/распределять stdin, передавая workers копии; наблюдать wake fd
 * вместе со stdin. Вернуть 1 при полном EOF, 0 при input/protocol failure, -1 при output wake.
 */
int bridge_receive(Worker *workers, int wake);


/* EN: Own two backend connections/workers and wake pipe for ports[2]. Return
 * process status 0/2. Stop and join every successfully started worker on all exits.
 * RU: Владеть двумя connections/workers и wake pipe для ports[2]. Вернуть
 * process status 0/2. Остановить/join всех запущенных workers при любом выходе.
 */
int bridge_run(const unsigned *ports);
#endif
