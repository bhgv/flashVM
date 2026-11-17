/* Copyright (c) 2026 bhgv
 * EN: Attached terminal Styx root to separate requester/provider launch capabilities.
 * RU: Подключённый Styx-корень терминала в раздельные requester/provider launch capabilities.
 */
#ifndef STYX_LAUNCH_REQUEST_H
#define STYX_LAUNCH_REQUEST_H
#include "client.h"
#include "launch/ticket.h"


/* EN: Consume one terminal client ref on every outcome; owned requester ticket/NULL.
 * Required request output receives a separate owned provider capability, NULL on failure;
 * caller supplies empty output storage. Borrow spec through return, with terminal == NULL.
 * spec.flashvm MUST be NULL: ordinary requests do not grant a FlashVM terminal.
 * Copy description/arguments using ordinary launch ticket rules.
 * Native metadata I/O captures/validates a live directory root; current cancellation
 * is respected before publication. Requires a cancellable native PHY client and a
 * READ|EXEC access ceiling (WRITE optional). Caller chooses vocabulary beforehand.
 * No dialing/authentication, automatic bidir, queue admission, guest execution or
 * wire serialization. Caller routes request through trusted admission and limits
 * concurrent acquisitions. Both peer wrappers remain native, never network pointers.
 * RU: Поглотить terminal ref всегда; свой ticket/NULL и отдельный owned request,
 * output при отказе NULL. Spec borrowed, terminal обязан быть NULL. Root capture и
 * metadata — native I/O с отменой; cancellable PHY и READ|EXEC ceiling обязательны.
 * spec.flashvm обязан быть NULL; FlashVM требует отдельного bidir constructor.
 * Description/arguments копируются обычными ticket rules. Без dial/auth,
 * auto-bidir, очереди admission, guest execution и wire serialization; quotas у host. */
DVM_LaunchTicket *styxlaunchrequest(StyxClient *terminal, const DVM_LaunchSpec *spec,
    unsigned access, DVM_LaunchRequest **request);

/* EN/RU: Borrow explicit FlashVM endpoint names/offer source through constructor return.
 * Names identify the already granted live terminal VFS; no protocol/type inference.
 * Borrow explicit FlashVM names/offers до return; только granted terminal VFS. */
typedef struct StyxTerminalPaths
{
    const char *receive; /* Borrowed canonical FlashVM-to-Dis control/data endpoint. */
    const char *send; /* Borrowed canonical Dis-to-FlashVM control/data endpoint; may equal receive. */
    DVM_MovieOffer *offers; /* Optional borrowed native extra-channel announcement source. */
} StyxTerminalPaths;


/* EN: FlashVM-terminal request, mandatory explicit bidir client plus READ|WRITE|EXEC ceiling.
 * Consume one terminal ref on every outcome, return owned ticket/request as above.
 * spec.terminal/spec.flashvm MUST be NULL. Copy paths and pin the SAME captured
 * root for the instance namespace and terminal channels. No separate ordinary
 * connection can be substituted for terminal traffic. Host must start both reverse
 * server/client roles; the local bidir marker does not prove peer liveness/auth.
 * RU: Запрос с FlashVM-терминалом требует bidir client и READ|WRITE|EXEC ceiling.
 * Terminal ref поглощается всегда; свои ticket/request. spec.terminal/flashvm NULL.
 * Пути копируются; один captured root удерживается для VFS и terminal channels.
 * Host запускает обе роли; local marker не доказывает peer liveness/auth. */
DVM_LaunchTicket *styxbidirlaunchrequest(StyxClient *terminal, const DVM_LaunchSpec *spec,
    unsigned access, const StyxTerminalPaths *paths, DVM_LaunchRequest **request);
#endif
