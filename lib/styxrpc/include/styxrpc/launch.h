/* Copyright (c) 2026 bhgv
 * EN: Host-side caller for a DisVM one-shot Unibus launch service.
 * RU: Вызов одноразового сервиса запуска DisVM из нативного хоста.
 */
#ifndef STYXRPC_LAUNCH_H
#define STYXRPC_LAUNCH_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct StyxClient; /* Borrowed from the installed Styx client. */


/* EN: Separate confirmed delivery from execution completion and uncertainty.
 * RU: Различать подтверждение доставки, завершение и неопределённость. */
typedef enum StyxRpcLaunchStatus
{
    STYXRPC_LAUNCH_NOT_SENT = -1, /* No launch payload was acknowledged or may have run. */
    STYXRPC_LAUNCH_UNKNOWN = 0,   /* Execution may have reached the peer. Do not replay. */
    STYXRPC_LAUNCH_PENDING = 1,   /* Acknowledged delivery, result not yet available. */
    STYXRPC_LAUNCH_OK = 2,        /* Program reported successful completion. */
    STYXRPC_LAUNCH_FAILED = 3     /* Program reported failed completion. */
} StyxRpcLaunchStatus;


/* EN: Submit one startup S-record on an attached raw-profile service. Scratch
 * is disjoint from arguments. Does not retry an uncertain write.
 * RU: Подать одну стартовую S-запись сервису с профилем raw. Буферы
 * не пересекаются. Не повторяет неподтверждённую запись. */
StyxRpcLaunchStatus styxrpc_launch_start(struct StyxClient *client, const void *arguments, size_t size,
    void *scratch, size_t capacity);


/* EN: Read the result on that same one-shot connection. Call only after start
 * returned PENDING. Accept one status snapshot; scheduling stays with the host.
 * RU: Прочитать результат на том же одноразовом соединении после PENDING.
 * Один снимок состояния; графиком опроса управляет вызывающий. */
StyxRpcLaunchStatus styxrpc_launch_status(struct StyxClient *client);

#ifdef __cplusplus
}
#endif
#endif
