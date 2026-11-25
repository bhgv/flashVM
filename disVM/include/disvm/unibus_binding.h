/* Copyright (c) 2026 bhgv
 * EN: Native per-launch Unibus capability, independent of either VM heap and wire protocol.
 * RU: Native-capability Unibus для одного запуска, без связи с кучами VM или wire-протоколом.
 */
#ifndef DISVM_UNIBUS_BINDING_H
#define DISVM_UNIBUS_BINDING_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UnibusBinding UnibusBinding; /* Owned native capability; retain before sharing. */
#define UNIBUS_INHERIT 1U /* Explicitly delegate this capability to subsequently spawned children. */
#define UNIBUS_OWNER_PORTS 2U /* Optional policy: ordinary attach/detach remain with the original owner. */
#define UNIBUS_CLOSE_WITH_OWNER 4U /* Optional policy: original owner exit revokes inherited capabilities. */


/* EN/RU: Native callback selectors only; these are not wire opcodes or new .m exports.
 * Только native-селекторы callback, не wire-opcodes и не новые экспорты .m.
 */
enum
{
    UNIBUS_INK, /* Existing ink(token) call. */
    UNIBUS_INKADMIT, /* Existing inkadmit(token, accepted) call. */
    UNIBUS_INKDONE, /* Existing inkdone(token, drawing) call. */
    UNIBUS_LINES, /* Existing linenumbers() call. */
    UNIBUS_ROWS, /* Existing rows() call. */
    UNIBUS_ATTACH, /* Ordinary port registration; OWNER_PORTS is optional policy. */
    UNIBUS_ATTACHBULK, /* Delegated private geometry port registration. */
    UNIBUS_DETACH, /* Ordinary port detachment; OWNER_PORTS is optional policy. */
    UNIBUS_FRESH, /* Existing fresh() call. */
    UNIBUS_PATH, /* Existing path() call. */
    UNIBUS_RPCOPEN, /* Open one addressed RPC exchange on this launch's session. */
    UNIBUS_RPCSEND, /* Send one copied envelope. */
    UNIBUS_RPCRECV, /* Poll and receive one copied envelope. */
    UNIBUS_RPCCLOSE, /* Retire a launch-owned exchange. */
    UNIBUS_METHODS /* Bounds native selector validation. */
};


/* EN/RU: Native state is shared only by explicit inheritance; callbacks synchronize it themselves.
 * Native-состояние разделяется только при явном наследовании; callbacks сами синхронизируют его.
 */
typedef struct UnibusOps
{
    int (*invoke)(void *state, int pid, unsigned method, void *frame); /* Owner-local typed .m frame, borrowed only during call; 1 handled/0 unavailable. */
    void (*retire)(void *state); /* Once-only cancellation on explicit revoke/final release or owner policy; no VM access or exceptions. */
    void (*destroy)(void *state); /* Optional final native disposal after all references/calls retire; no VM access or exceptions. */
} UnibusOps;


/* EN: Copy ops and adopt native state on success, returning owned binding/NULL.
 * invoke is mandatory; flags combine the three policies above. By default inherited references
 * keep the service alive after the original owner exits, like ordinary inherited FD ownership.
 * State must outlive all references if destroy is NULL.
 * Callbacks never retain/serialize frame or managed addresses; invoke runs on the calling guest's
 * active owner and may use that owner's ordinary generated builtin ABI/error boundary.
 * RU: Копировать ops, принять native state при успехе, вернуть свой binding/NULL. invoke обязателен.
 * Наследованные refs по умолчанию сохраняют сервис после выхода первоначального owner, как FD.
 * Без destroy состояние живёт дольше всех refs. frame заимствован только на вызов;
 * callback работает на текущем guest owner, не сохраняет и не передаёт managed-адреса.
 */
UnibusBinding *unibusbinding(const UnibusOps *ops, void *state, unsigned flags);


/* EN/RU: Retain live borrowed capability, 1/0; release consumes one owned reference, NULL harmless.
 * Удержать живую capability, 1/0; release поглощает один ref, NULL допустим.
 */
int unibusretain(UnibusBinding *binding);
void unibusrelease(UnibusBinding *binding);


/* EN/RU: Claim once for a nonzero opaque native owner identity before PID publication, 1/0; no callbacks, I/O or guest execution.
 * Назначить один раз ненулевую opaque native owner identity до init, 1/0; без callbacks/I/O/guest-кода.
 */
int unibusclaim(UnibusBinding *binding, uintptr_t owner);


/* EN/RU: Return one retained reference only when explicit inheritance is live; otherwise NULL.
 * Выдать один retained ref только при разрешённом живом наследовании, иначе NULL.
 */
UnibusBinding *unibusinherit(UnibusBinding *binding);


/* EN/RU: 1 handled, 0 invalid, -1 foreign root operation, -2 retired, -3 unavailable method.
 * No retry/fallback; caller owns live ref and its active .m frame. User callback may raise only
 * through that caller's existing owner error boundary. / Без повторов и fallback, ref/frame у caller.
 */
int unibusinvoke(UnibusBinding *binding, uintptr_t caller, int pid, unsigned method, void *frame);


/* EN/RU: Explicit root revocation, idempotent; foreign child identity does not revoke shared capability.
 * Отозвать при завершении root, повтор допустим; чужая child identity не отзывает общую capability.
 */
void unibusretire(UnibusBinding *binding, uintptr_t caller);


/* EN/RU: Original process retirement; revoke only with CLOSE_WITH_OWNER, otherwise delegated refs
 * remain live. Record owner death so a reused native address cannot regain owner-only rights.
 * Завершение исходного process: отзыв только с CLOSE_WITH_OWNER; иначе refs потомков живы.
 * Запомнить завершение owner, чтобы повтор адреса не восстановил owner-only права.
 */
void unibusownerretired(UnibusBinding *binding, uintptr_t caller);
#ifdef __cplusplus
}
#endif
#endif
