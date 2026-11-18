/* Copyright (c) 2026 bhgv
 * EN: Ordered broker private ownership, directional offers, per-tag renewal and native FIFO workers.
 * RU: Закрытое состояние брокера: владение, предложения направлений, сбросы тегов и native workers.
 */
#ifndef STYX_ORDERED_INTERNAL_H
#define STYX_ORDERED_INTERNAL_H
#include "../../internal.h"
#include "broker.h"
#include "flashvm/sordctl.h"


/* EN/RU: One owned outer payload; retained through physical admission, no re-encoding retries.
 * Одно собственное внешнее тело; сохраняется до приёма транспортом, без повторного кодирования номера.
 */
typedef struct OrderedMessage
{
    struct OrderedMessage *next; /* Owned FIFO continuation. */
    unsigned long tag; /* Actual outer tag0 or negotiated lane. */
    size_t size; /* Exact copied body extent. */
    unsigned char bytes[]; /* Private encoded packet/service payload. */
} OrderedMessage;


/* EN/RU: One immutable client grant and its directional renewal state, protected by peer lock.
 * Один неизменяемый клиентский grant и состояние сбросов направлений под mutex peer.
 */
struct StyxOrderedClient
{
    struct StyxOrderedClient *next; /* Owned registry continuation. */
    StyxOrdered *peer; /* Borrowed owner until joined disposal. */
    unsigned long tag; /* Fixed inner tag, not root service authority. */
    int sent; /* TX boundary exists. */
    unsigned long long last; /* Last spooled TX number. */
    unsigned long long renewal; /* Greatest initiated local reset ID, never reused. */
    int tx_pending; /* Only this tag's encoder is held by its reset proposal. */
    FVM_SOrderControl tx_request; /* Exact pending/latest TX renewal request. */
    FVM_SOrderControl rx_request; /* Exact pending/latest RX renewal request. */
    int rx_pending; /* Old RX boundary must still be consumed. */
    OrderedMessage *rx_reply; /* Preallocated ACK, private until boundary ready. */
};


/* EN/RU: Owned native broker, no guest pointers; mutex order is peer then sequence engine.
 * Свой native-брокер без гостевых указателей; порядок mutex: peer, затем sequence engine.
 */
struct StyxOrdered
{
    pthread_mutex_t lock; /* Offers, output, capabilities and first reason. */
    pthread_cond_t changed; /* Native outbox/readiness notification, monotonic. */
    pthread_t workers[2]; /* Independent RX dispatcher and retained TX outbox. */
    unsigned started; /* Successfully created workers, joined before free. */
    int reason; /* Sticky transport reason, zero while live. */
    StyxBidirControl *control; /* Owned ordinary S-record control, framing never switched globally. */
    StyxOrder *engine; /* Owned per-inner-tag queues/counters. */
    StyxOrderedClient *clients; /* Owned immutable predeclared application grants. */
    unsigned limit; /* Local inner-body ceiling. */
    unsigned long long window; /* Local future-number displacement ceiling. */
    FVM_SOrderControl offer; /* Latest TX proposal. */
    FVM_SOrderControl answer; /* Latest received TX resolution. */
    FVM_SOrderControl remote; /* Latest peer offer, exact duplicate detection. */
    FVM_SOrderControl reply; /* Cached peer offer outcome. */
    int tx_pending; /* TX profile proposal awaits resolution. */
    int active[2]; /* Independent negotiated RX/TX directions. */
    unsigned long lanes[2]; /* Directional outer wrapper tags. */
    unsigned limits[2]; /* Negotiated inner body ceilings. */
    OrderedMessage *head; /* Owned dynamic TX queue, head retained through sendwait. */
    OrderedMessage *tail; /* Borrowed newest TX node. */
    OrderedMessage *services; /* Owned unknown root0 inbox. */
    OrderedMessage *service_tail; /* Borrowed newest root0 inbox node. */
};


/* EN/RU: Portable magnitude conversion, explicit 32-bit limbs, no native serialization.
 * Перевод величины в явные 32-битные части, без native-сериализации.
 */
FVM_SOrdinal orderednumber(unsigned long long value);


/* EN/RU: Combine checked portable parts into a native scalar; no allocation.
 * Собрать проверенные переносимые части в native-число; без выделения памяти.
 */
unsigned long long orderedvalue(FVM_SOrdinal value);


/* EN/RU: Lookup immutable client under pinned owner, no allocation; NULL if not granted.
 * Поиск неизменяемого клиента, без выделения памяти; NULL, если grant отсутствует.
 */
StyxOrderedClient *orderedfind(StyxOrdered *peer, unsigned long tag);


/* EN/RU: Allocate copied outer body or encoded named service, owned node/NULL; no queue mutation.
 * Создать копию внешнего тела либо именованную службу, свой узел/NULL; очередь не меняется.
 */
OrderedMessage *orderedcopy(unsigned long tag, const void *bytes, size_t size);


/* EN/RU: Encode and allocate one named service body, owned result/NULL.
 * Кодировать и выделить тело именованной службы, свой результат/NULL.
 */
OrderedMessage *orderedcontrol(const FVM_SOrderControl *message);


/* EN/RU: Append owned TX node under peer lock; cannot fail or perform I/O.
 * Добавить свой TX-узел под mutex peer; без отказа и I/O.
 */
void orderedqueue(StyxOrdered *peer, OrderedMessage *message);


/* EN/RU: Compare checked service scalars, ignoring only command when requested; 1 equal/0 otherwise.
 * Сравнить скаляры службы, при необходимости кроме command; 1/0.
 */
int orderedequal(const FVM_SOrderControl *a, const FVM_SOrderControl *b, int command);


/* EN/RU: Handle named root0 under peer lock, 1 handled/0 malformed/-1 allocation failure; no wait or I/O.
 * Обработать именованную root0-службу под mutex peer: 1/0, без ожиданий и I/O.
 */
int orderedprofile(StyxOrdered *peer, const FVM_SOrderControl *message);


/* EN/RU: Apply one checked reset request/confirmation under peer lock; 1/0, -1 allocation failure.
 * Применить проверенный сброс/подтверждение под mutex peer: 1/0, -1 при отказе выделения.
 */
int orderedrenewal(StyxOrdered *peer, const FVM_SOrderControl *message);


/* EN/RU: Queue one TX renewal or ready RX ACK under peer lock; allocation precedes sequence mutation.
 * Поставить запрос TX либо готовое ACK RX под mutex peer; выделение предшествует изменению счётчика.
 */
int orderedrequest(StyxOrderedClient *client);


/* EN/RU: Queue a preallocated RX ACK only after selected old records were consumed, 1/0.
 * Поставить готовое RX ACK лишь после выдачи старых записей выбранного тега: 1/0.
 */
int orderedack(StyxOrderedClient *client);


/* EN/RU: Latch terminal reason, close sequence queues and stop control outside peer lock; no joins.
 * Зафиксировать завершение, закрыть sequence-очереди и control вне mutex peer; без join.
 */
void orderedend(StyxOrdered *peer, int reason);


/* EN/RU: Borrow peer until joined; RX dispatcher/TX sender entries never execute guest code.
 * Заимствовать peer до join; RX/TX workers не исполняют гостевой код.
 */
void *orderedreceive(void *argument);


/* EN/RU: Native sender borrows peer through join; only this worker submits retained TX nodes.
 * Native-отправитель заимствует peer до join; только он передаёт сохранённые TX-узлы.
 */
void *orderedtransmit(void *argument);
#endif
