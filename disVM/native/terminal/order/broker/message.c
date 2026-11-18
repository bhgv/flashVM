/* Copyright (c) 2026 bhgv
 * EN: Immutable broker messages and portable correlation values, no transport callbacks.
 * RU: Неизменяемые сообщения брокера и переносимые номера операций, без transport callbacks.
 */
#include "internal.h"


/* EN/RU: See internal.h; preserve exact unsigned64 magnitude in portable parts.
 * См. internal.h; сохранить точную величину unsigned64 в переносимых частях.
 */
FVM_SOrdinal orderednumber(unsigned long long value)
{
    FVM_SOrdinal result = {(unsigned long)(value >> 32), (unsigned long)(value & 0xffffffffULL)}; /* Copied scalar. */
    return result;
}


/* EN/RU: See internal.h; caller supplies checked limbs from codec/native state.
 * См. internal.h; caller передаёт проверенные части из кодека/native-состояния.
 */
unsigned long long orderedvalue(FVM_SOrdinal value)
{
    return ((unsigned long long)value.high << 32) | value.low;
}


/* EN/RU: See internal.h; immutable registry remains alive until all external users retire.
 * См. internal.h; неизменяемый registry живёт до завершения внешних users.
 */
StyxOrderedClient *orderedfind(StyxOrdered *peer, unsigned long tag)
{
    for (StyxOrderedClient *client = peer->clients; client; client = client->next)
    {
        if (client->tag == tag)
        {
            return client;
        }
    }
    return NULL;
}


/* EN/RU: See internal.h; NULL bytes reserves private uninitialized payload for caller to fill before queueing.
 * См. internal.h; NULL bytes резервирует закрытый payload, caller заполнит его до queueing.
 */
OrderedMessage *orderedcopy(unsigned long tag, const void *bytes, size_t size)
{
    if (size > (size_t)-1 - sizeof(OrderedMessage))
    {
        return NULL;
    }
    OrderedMessage *message = malloc(sizeof(*message) + size); /* Owned exact node. */
    if (message)
    {
        message->next = NULL;
        message->tag = tag;
        message->size = size;
        if (bytes && size)
        {
            memcpy(message->bytes, bytes, size);
        }
    }
    return message;
}


/* EN/RU: See internal.h; complete codec validation precedes heap publication.
 * См. internal.h; полная проверка кодека предшествует созданию узла.
 */
OrderedMessage *orderedcontrol(const FVM_SOrderControl *message)
{
    unsigned char bytes[FVM_SORDER_CTL_MAX]; /* Private named-service encoding. */
    size_t size; /* Complete body extent. */
    return fvm_sorder_control_write(message, bytes, sizeof(bytes), &size) == FVM_S_OK ? orderedcopy(0, bytes, size) : NULL;
}


/* EN/RU: See internal.h; node ownership transfers to outbox under peer mutex.
 * См. internal.h; владение узлом передаётся outbox под mutex peer.
 */
void orderedqueue(StyxOrdered *peer, OrderedMessage *message)
{
    if (peer->tail)
    {
        peer->tail->next = message;
    }
    else
    {
        peer->head = message;
    }
    peer->tail = message;
    pthread_cond_broadcast(&peer->changed);
}


/* EN/RU: See internal.h; avoid native padding/memcmp dependencies in correlation checks.
 * См. internal.h; не зависеть от padding структуры при сравнении операций.
 */
int orderedequal(const FVM_SOrderControl *a, const FVM_SOrderControl *b, int command)
{
    return (!command || a->command == b->command) && orderedvalue(a->operation) == orderedvalue(b->operation) &&
        a->lane == b->lane && a->limit == b->limit && orderedvalue(a->window) == orderedvalue(b->window) &&
        a->tag == b->tag && orderedvalue(a->boundary) == orderedvalue(b->boundary);
}
