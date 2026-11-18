/* Copyright (c) 2026 bhgv
 * EN: Bounded bidir terminal S-record transport, independent of event/proxy payload schemas.
 * RU: Bounded bidir terminal S-record transport независимо от event/proxy payload schemas.
 */
#ifndef STYX_BIDIR_CONTROL_H
#define STYX_BIDIR_CONTROL_H
#include "client.h"
#include "launch/terminal.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct StyxBidirControl StyxBidirControl; /* Owned native receive/transmit workers and queues. */
struct DVM_Cancel; /* Optional borrowed native cancellation for bounded TX admission. */
#define STYX_CONTROL_RECORD 1 /* Exactly one copied record. */
#define STYX_CONTROL_EMPTY 0 /* No queued record while live. */
#define STYX_CONTROL_END (-1) /* Terminal after draining confirmed RX records. */
#define STYX_CONTROL_SMALL (-2) /* Caller output too small; record remains queued. */
#define STYX_CONTROL_LIVE 0 /* Both directions active. */
#define STYX_CONTROL_STOPPED 1 /* Host explicitly stopped. */
#define STYX_CONTROL_EOF 2 /* Input ended. */
#define STYX_CONTROL_READ_FAILED 3 /* Read/cancellation/transport failure. */
#define STYX_CONTROL_WRITE_FAILED 4 /* Failed/uncertain output, never replayed. */
#define STYX_CONTROL_PROTOCOL 5 /* Framing/checksum/bounds failure. */

struct DVM_CancelTap; /* Owned native subscription, retired with dvm_cancel_unsubscribe. */


/* EN/RU: Subscribe to sticky local transport/route retirement, owned tap/NULL; state adopted only on success.
 * Native nonwaiting callback may run before return, without internal queue locks, possibly more than once.
 * Use dvm_cancel_unsubscribe; release(state) follows admitted callbacks and can outlive controlfree.
 * Borrow control through setup; callback must not access it after its owner retires. No queue consumption.
 * Подписка на local retirement, свой tap/NULL; state принят при успехе, release после callbacks.
 * Callback native без ожидания/VM/I/O; sticky replay возможен до return, без queue locks.
 */
struct DVM_CancelTap *styxbidircontrolwatch(StyxBidirControl *control, void (*receive)(void *),
    void *state, void (*release)(void *));


/* EN/RU: Borrow client and descriptor through return; owned control/NULL, client retained on success.
 * Requires this SDK's native Styx grant on the exact client; enforce its immutable READ/WRITE
 * ceiling plus directory EXEC/search. Clone the captured root; canonical RX/TX stay relative
 * to it, file roots allow dot only. Remote Topen decides current permission. No ambient fallback.
 * Explicit bidir/watchable client, limit1..65536 body bytes, slots1..32 per direction. Own root
 * and independent RX/TX fids; sticky client-terminal watch, two native workers, no guest/SDL
 * callbacks or new wire schemas. Refuse watch failure/observed terminal during setup.
 * Borrow client/descriptor до return; свой control/NULL. Только native Styx grant exact client,
 * READ/WRITE ceiling и directory search; captured root, file root только dot. Текущие права
 * решает Topen. Scope отменяет setup; rollback завершает своих workers и поглощает fids. */
StyxBidirControl *styxbidircontrol(StyxClient *client, const DVM_TerminalChannels *channels,
    unsigned limit, unsigned slots);


/* EN/RU: Nonblocking bounded enqueue of one encoded S-record;1 accepted/0 full, invalid or terminal.
 * Borrow payload through return, copy before acceptance; never promises peer execution/delivery.
 * Tag0 is ordinary control content, not interpreted here; native callers may race next/state/stop.
 * Nonblocking enqueue одной copied S-record;1/0, без обещания delivery/peer execution. */
int styxbidircontrolsend(StyxBidirControl *control, unsigned long tag, const void *payload, size_t size);


/* EN/RU: Copy one complete record after bounded native TX-capacity wait, 1 accepted/0 refused.
 * Payload/control/cancel borrowed through return; timeout 0..120000ms. Never retry an accepted frame.
 * Cancellation wakes only this waiter and does not retract queued records or stop the control.
 * Native-ожидание TX capacity и одна копия S-записи, 1/0; отмена не закрывает общий control.
 */
int styxbidircontrolsendwait(StyxBidirControl *control, unsigned long tag, const void *payload,
    size_t size, unsigned timeout, struct DVM_Cancel *cancel);


/* EN/RU: Copy exactly one verified record; EMPTY/END/SMALL or RECORD. On SMALL only size changes.
 * On RECORD fill tag/size and payload (NULL allowed for empty); no record batch, no callbacks.
 * Inputs/output borrowed; one consumer, may race send/stop. Tag0 is delivered like any other.
 * Выдать одну verified record; EMPTY/END/SMALL/RECORD. SMALL меняет только size, не удаляет record. */
int styxbidircontrolnext(StyxBidirControl *control, unsigned long *tag, void *payload, size_t capacity, size_t *size);


/* EN/RU: Optional native consumer wait, monotonic budget0..120000ms;1 record queued/0 timeout or end.
 * Does not consume a record; inspect next/state. No guest dispatch or callback.
 * Native consumer wait;1 queued/0 timeout/end, record не потребляется, без guest dispatch. */
int styxbidircontrolwait(StyxBidirControl *control, unsigned timeout);


/* EN/RU: Native wait for queued output acknowledgements within one monotonic budget0..120000ms;
 * 1 drained/0 pending or terminal. No guest work; timeout retains queue/prefix. Not peer execution ACK.
 * Ждать output ACK под единым monotonic budget;1 drained/0 pending/terminal, без guest work/replay. */
int styxbidircontrolflush(StyxBidirControl *control, unsigned timeout);


/* EN/RU: Copied terminal reason above; invalid returns READ_FAILED. No I/O; queue remains drainable.
 * Скопировать terminal reason без I/O; buffered RX можно дочитать. */
int styxbidircontrolstate(StyxBidirControl *control);


/* EN/RU: Borrow immutable exact client association while control stays pinned; NULL for invalid.
 * No ownership/permission transfer, I/O or reverse client discovery. / Заимствовать exact client,
 * без передачи прав/владения, I/O или поиска встречного соединения.
 */
StyxClient *styxbidircontrolclient(StyxBidirControl *control);


/* EN/RU: Walk a canonical relative name from the owned grant root; owned unopened fid/NULL.
 * Borrow control/path through scoped native I/O. No ambient-root fallback; a file root allows dot only.
 * Host pins control through all calls and retires them before free; never open/clunk its private root.
 * Пройти canonical relative name от grant root; свой unopened fid/NULL. File root — только dot,
 * без ambient fallback. Caller удерживает control до return и завершает все calls до free.
 */
StyxFid *styxbidircontrolwalk(StyxBidirControl *control, const char *relative);


/* EN/RU: Request native cancellation, no joins/client shutdown; NULL harmless. Discards unsent TX.
 * Запросить native cancel без joins/client shutdown; unsent TX отбрасывается. */
void styxbidircontrolstop(StyxBidirControl *control);


/* EN/RU: Consume after external callers retire; stop/join only its two native workers, then clunk/free.
 * Never call on owned workers or retirement callbacks; shared client is not shut down. NULL harmless.
 * Поглотить после external callers; join только своих workers, shared client не закрывать. */
void styxbidircontrolfree(StyxBidirControl *control);
#ifdef __cplusplus
}
#endif
#endif
