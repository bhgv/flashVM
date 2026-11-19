/* Copyright (c) 2026 bhgv
 * EN: Sole transport RX owner; validate outer service/routing records, then
 * copy each client's own message/error marker to its independently serviced FIFO.
 * RU: Единственный transport RX owner: проверить служебные/маршрутные записи,
 * затем поставить сообщение/маркер ошибки в отдельную FIFO нужного клиента.
 * standalone-reviewed: RX never calls the physical output path for client replies.
 */
#include "bridge.h"
#include "flashvm/sstream.h"
#include "flashvm/sbindings.h"
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>


/* EN: record borrows decoder storage; bindings/workers belong to the session.
 * Return 1 after service acceptance or queue copy, 0 for carrier/profile/admission
 * failure. Invalid inner client frames become queue-local errors, not RX writes.
 * RU: record заимствует decoder storage; bindings/workers принадлежат сессии.
 * Вернуть 1 после принятия службы/копии в FIFO, 0 при carrier/profile/admission
 * failure. Неверный inner frame становится локальной ошибкой, не RX-записью.
 */
static int dispatch(FVM_SRecord record, FVM_SBindings *bindings, Worker *workers)
{
    size_t used;
    if (!record.tag)
    {
        unsigned char encoded[1024]; /* Only this example's bounded service assignment profile is accepted. */
        size_t size;
        return fvm_srecord_write(0, record.payload, record.size, encoded, sizeof(encoded), &size) == FVM_S_OK &&
            fvm_sbindings_apply(bindings, encoded, size, &used) == FVM_S_OK && used == size;
    }
    FVM_SBinding assignment;
    if ((record.tag != 7 && record.tag != 8) ||
        fvm_sbindings_lookup(bindings, record.tag, &assignment) != FVM_S_OK ||
        strcmp(assignment.type, "example.9p.v1") || assignment.version != 1)
    {
        return 0; /* Unknown transport routing/schema is service-level misuse, not a client request. */
    }
    FVM_SRecord request;
    int invalid = fvm_srecord_read(record.payload, record.size, BRIDGE_LIMIT + 32, &request, &used) != FVM_S_OK;
    if (!invalid)
    {
        invalid = used != record.size || request.tag != 1 || request.size < 7 ||
            request.size > BRIDGE_LIMIT || bridge_length(request.payload) != request.size;
    }
    if (!bridge_enqueue(&workers[record.tag - 7], invalid ? NULL : request.payload, invalid ? 0 : request.size, invalid))
    {
        fprintf(stderr, "client %lu queue admission failed\n", record.tag); /* No silent quota drop or RX-thread client reply. */
        return 0;
    }
    return 1; /* The decoder may now overwrite the borrowed input: FIFO owns its copy. */
}


/* EN/RU: See bridge.h; complete EOF drains accepted messages, a wake interrupts idle RX.
 * См. bridge.h; полный EOF допускает drain, wake прерывает ожидающий RX.
 */
int bridge_receive(Worker *workers, int wake)
{
    unsigned char storage[BRIDGE_LIMIT + 256]; /* Decoder storage is separate from raw read fragments. */
    unsigned char input[4096]; /* A physical read may contain partial, complete or several S-records. */
    FVM_SStream stream;
    FVM_SBinding slots[BRIDGE_CLIENTS];
    FVM_SBindings bindings;
    if (fvm_sstream_init(&stream, storage, sizeof(storage), sizeof(storage)) != FVM_S_OK ||
        fvm_sbindings_init(&bindings, slots, BRIDGE_CLIENTS) != FVM_S_OK)
    {
        return 0;
    }
    int incomplete = 0; /* EOF is valid only between complete checked outer records. */
    for (;;)
    {
        struct pollfd descriptors[2] = {{STDIN_FILENO, POLLIN, 0}, {wake, POLLIN, 0}};
        int ready = poll(descriptors, 2, -1); /* Native RX wait, independent of every client's backend/queue wait. */
        if (ready < 0 && errno == EINTR)
        {
            continue;
        }
        if (ready < 0)
        {
            perror("gateway input poll");
            return 0;
        }
        if (descriptors[1].revents)
        {
            return -1; /* Worker observed terminal physical output loss; wake is private, not client traffic. */
        }
        if (descriptors[0].revents & (POLLERR | POLLNVAL))
        {
            return 0;
        }
        ssize_t amount = read(STDIN_FILENO, input, sizeof(input)); /* Sole RX ownership prevents another reader from stealing readiness. */
        if (amount < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
        {
            continue;
        }
        if (amount <= 0)
        {
            return amount == 0 && !incomplete; /* Reject truncated framing rather than treating it as normal shutdown. */
        }
        size_t cursor = 0;
        while (cursor < (size_t)amount)
        {
            FVM_SRecord record;
            size_t consumed = 0;
            FVM_SResult result = fvm_sstream_push(&stream, input + cursor, (size_t)amount - cursor, &record, &consumed);
            if (!consumed || (result != FVM_S_OK && result != FVM_S_MORE))
            {
                return 0; /* No busy retry, skipping bad bytes or speculative resynchronization. */
            }
            cursor += consumed; /* The decoder already retained consumed fragments; never submit them twice. */
            incomplete = result != FVM_S_OK;
            if (result == FVM_S_OK &&
                (!dispatch(record, &bindings, workers) || fvm_sstream_reset(&stream) != FVM_S_OK))
            {
                return 0;
            }
        }
    }
}
