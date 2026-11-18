/* Copyright (c) 2026 bhgv
 * EN: Ordered native root services and reliable copied service outbox, without assigning a wire schema.
 * RU: Упорядоченные native root services и outbox копий без назначения нового wire schema.
 */
#ifndef STYX_SCOPE_SERVICE_H
#define STYX_SCOPE_SERVICE_H
#include "scope.h"
#ifdef __cplusplus
extern "C" {
#endif
#define STYX_SCOPE_PASS 0       /* Unrecognized service remains in native root inbox. */
#define STYX_SCOPE_HANDLED 1    /* Native service consumed this root record before next frame. */
#define STYX_SCOPE_SERVICE_ERROR (-1) /* Malformed/unsafe service retires scoped broker. */


/* EN/RU: Native broker-thread handler of one root tag0, never called for client/group-local zero.
 * Borrow root/body through return, no waiting, guest calls, joins, direct transport/provider I/O or rootfree.
 * May atomically assign scopes, confirm operations or queue native service output; return PASS/HANDLED/ERROR.
 * Native broker handler root tag0; root/body borrowed, без waits/guest/I/O/free, допустимы native atomic ops/outbox.
 */
typedef int (*StyxScopeService)(void *state, StyxScope *root, const void *bytes, size_t size);


/* EN/RU: Adopt state only on successful handler replacement, 1/0; NULL handler clears, requires NULL state/release.
 * Replacement never waits for in-flight handler; release occurs outside locks after final invocation.
 * Host pins root through setup and retires external callers before free. Native hook is not peer authorization.
 * State принят лишь при успехе, replacement не ждёт callback; release вне locks после final invocation.
 */
int styxscopeservicehandler(StyxScope *root, StyxScopeService handler, void *state, void (*release)(void *));


/* EN/RU: Native-only copy one root tag0 body to dynamic service outbox, 1 admitted/0 refused, no wire I/O/wait.
 * Root must be native root authority, payload borrowed through return. Unrelated application TX may interleave;
 * already accepted frames retain FIFO order. Failure leaves input owned by caller. Not delivery/execution ACK.
 * Native-only копия root tag0 в dynamic outbox, 1/0, без I/O/wait; не delivery/execution ACK.
 */
int styxscopeservicesend(StyxScope *root, const void *bytes, size_t size);
#ifdef __cplusplus
}
#endif
#endif
