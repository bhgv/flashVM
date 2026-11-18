/* Copyright (c) 2026 bhgv
 * EN: Dedicated native terminal dispatcher with separate bounded UI and general mailboxes.
 * RU: Отдельный native-диспетчер терминала с ограниченными UI и общей почтовыми очередями.
 */
#ifndef STYX_BIDIR_ROUTE_H
#define STYX_BIDIR_ROUTE_H
#include "ui/frame.h"
#include "flashvm/sbindings.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct StyxBidirRoute StyxBidirRoute; /* Owns its control and one native dispatcher. */


/* EN: Borrow client/channels/type during setup; return owned route or NULL.
 * client/channels MUST refer to the same original bidir grant, as required by control.h.
 * The host must have negotiated the existing tag-zero binding-declaration profile for RX.
 * type/version identify its UI schema, never a capability or a new wire identity assigned here.
 * Each valid declaration updates RX bindings atomically and is also delivered to the general queue.
 * Malformed/conflicting declarations or malformed bound UI stop routing; earlier queues drain.
 * limit/slots have control's bounds; each mailbox has slots entries. A full mailbox backpressures
 * this native dispatcher only; no guest callbacks, polling, suspension or queue drops.
 * RU: Заимствовать аргументы, вернуть свой route/NULL. client/channels — один исходный grant.
 * Требуется заранее
 * согласованный RX-профиль объявлений bindings в tag0. type/version — согласованная схема UI.
 * Декларации обновляют RX и выдаются общей очередью. Ошибки прекращают приём, прежние записи
 * доступны для чтения. Заполненная очередь задерживает только собственный native-диспетчер.
 */
StyxBidirRoute *styxbidirbindingroute(StyxClient *client, const DVM_TerminalChannels *channels,
    unsigned limit, unsigned slots, const char *type, unsigned long version);


/* EN: Nonblocking copy of one UI message to output; RECORD/EMPTY/END, output unchanged otherwise.
 * Borrow route/output, one consumer per mailbox; may race send/stop and the general consumer.
 * RU: Неблокирующая копия одного сообщения UI; RECORD/EMPTY/END. output меняется только при RECORD.
 */
int styxbidirrouteui(StyxBidirRoute *route, DVM_UIMessage *output);


/* EN: Copy one general record, including declarations and unknown types; control's next statuses.
 * SMALL changes only size and preserves head. One consumer, route/output borrowed through return.
 * RU: Выдать одну общую запись, включая декларации и неизвестные типы. При SMALL меняется только
 * size, запись остаётся в очереди. По одному потребителю на очередь, аргументы заимствованы.
 */
int styxbidirroutenext(StyxBidirRoute *route, unsigned long *tag, void *bytes, size_t capacity, size_t *size);


/* EN: Native-only readiness wait for ui=1 or general=0, monotonic budget 0..120000ms.
 * Return 1 for a queued item, 0 for timeout/end/invalid; consume nothing, call ui/next afterwards.
 * RU: Ожидание native-потребителя: ui=1 либо общая очередь=0, 0..120000 мс. 1 — есть запись,
 * 0 — timeout/end/ошибка. Не потребляет записи и не исполняет guest-код.
 */
int styxbidirroutewait(StyxBidirRoute *route, int ui, unsigned timeout);


/* EN: Nonblocking TX copy on the host's independently established outgoing UI tag; 1/0 accepted.
 * RX declarations never grant a TX tag. Route/message borrowed; same guarantees as uisend.
 * RU: Неблокирующее копирование TX по отдельно согласованному исходящему tag; 1/0 принято.
 * Объявления RX не определяют TX. Владение и гарантии те же, что у uisend.
 */
int styxbidirrouteuisend(StyxBidirRoute *route, unsigned long tag, const DVM_UIMessage *message);


/* EN/RU: Copy one general/control TX record; same acceptance/ownership as controlsend.
 * Скопировать одну общую/control TX-запись, принятие и владение как у controlsend.
 */
int styxbidirroutesend(StyxBidirRoute *route, unsigned long tag, const void *bytes, size_t size);


/* EN/RU: Native TX acknowledgement wait; same timeout/ownership as controlflush, not execution ACK.
 * Native ожидание подтверждения TX как controlflush, не подтверждение исполнения приложения.
 */
int styxbidirrouteflush(StyxBidirRoute *route, unsigned timeout);


/* EN/RU: First routing/transport terminal reason, or LIVE; no I/O or queue consumption.
 * Первая terminal reason либо LIVE, без I/O и потребления очередей.
 */
int styxbidirroutestate(StyxBidirRoute *route);


/* EN/RU: Immutable configured record limit, or 0 for NULL; no I/O or mutable state access.
 * Неизменяемый лимит записи либо 0 для NULL, без I/O и доступа к изменяемому состоянию.
 */
unsigned styxbidirroutelimit(StyxBidirRoute *route);


/* EN: Request cancellation without joining; NULL harmless. Buffered RX remains drainable.
 * RU: Запросить отмену без join. NULL допустим, уже принятые RX-записи доступны для чтения.
 */
void styxbidirroutestop(StyxBidirRoute *route);


/* EN: After external users retire, stop and join only owned native workers, release route.
 * Shared client remains owned by its host. NULL harmless; never call from the dispatcher itself.
 * RU: После завершения внешних пользователей остановить и join только свои native-потоки,
 * освободить route. Общий client остаётся у host. NULL допустим.
 */
void styxbidirroutefree(StyxBidirRoute *route);
#ifdef __cplusplus
}
#endif
#endif
