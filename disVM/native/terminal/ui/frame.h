/* Copyright (c) 2026 bhgv
 * EN: Existing IDE fvd1 messages on an explicitly bound bidir terminal route.
 * RU: Существующие сообщения IDE fvd1 через явно привязанный bidir-маршрут.
 */
#ifndef STYX_TERMINAL_UI_FRAME_H
#define STYX_TERMINAL_UI_FRAME_H
#include "control.h"
#include "disvm/ui_message.h"

#define STYX_UI_MESSAGE 1  /* One validated copied message. */
#define STYX_UI_OTHER 0    /* Another route; caller retains the record for its dispatcher. */
#define STYX_UI_INVALID -1 /* Invalid route or malformed message; output untouched. */


/* EN: Borrow control/message; copy one fvd1 frame to the native TX queue.
 * tag is a nonzero UI binding established by the host's dispatcher, not a reserved wire tag.
 * Return 1 for queue acceptance, 0 for invalid/full/ended; never block or promise delivery.
 * Caller keeps control alive; permitted concurrently with native transport I/O.
 * RU: Заимствовать control/message, скопировать fvd1 в TX. tag — согласованная
 * ненулевая UI-привязка. 1 — принято очередью, 0 — отказ; без ожидания и обещания доставки.
 */
int styxbidiruisend(StyxBidirControl *control, unsigned long tag, const DVM_UIMessage *message);


/* EN: Decode one record already obtained by the sole terminal dispatcher.
 * expected is its established nonzero UI tag; tag/bytes/size describe this borrowed record.
 * MESSAGE publishes a copied message; OTHER/INVALID preserve output and all transport queues.
 * No reading, waiting, callbacks or hidden consumer; other routes remain with the dispatcher.
 * RU: Декодировать одну запись единственного диспетчера. expected — согласованный UI-tag.
 * MESSAGE выдаёт копию; OTHER/INVALID сохраняют output. Очереди не читаются и не изменяются.
 */
int styxbidiruidecode(unsigned long expected, unsigned long tag, const void *bytes,
    size_t size, DVM_UIMessage *output);
#endif
