/* Copyright (c) 2026 bhgv
 * EN: Host-side caller for a DisVM one-shot Unibus launch service.
 * RU: Вызов одноразового сервиса запуска DisVM из нативного хоста.
 */
#ifndef STYXRPC_LAUNCH_H
#define STYXRPC_LAUNCH_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_NAMED_FRAME_MAX 64U /* Maximum complete persistent-launch frame. */
#define STYXRPC_NAMED_NAME_MAX 31U  /* Maximum public program basename in bytes. */

struct StyxClient; /* Borrowed from the installed Styx client. */
struct StyxFid;    /* Owned single request on a persistent connection. */


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

/* EN: Identify already published reverse .prog entries for one named launch.
 * RU: Выбрать опубликованные встречные записи .prog именованного запуска. */
typedef struct StyxRpcProgram
{
    uint32_t pid;        /* Nonzero ID in the caller's reverse Styx tree. */
    uint32_t parent_pid; /* Optional distinct parent .prog entry, zero when absent. */
    uint32_t hidden_fds; /* Hide inherited descriptors selected by bit number. */
} StyxRpcProgram;


/* EN: Borrow client, program and arguments, submit once on programs/.NAME.dis.
 * scratch is disjoint from arguments. PENDING gives request one owned fid,
 * which pins this launch through status or close. Failure leaves request NULL.
 * RU: Заимствовать client, program и arguments, подать один запрос programs/.NAME.dis.
 * scratch не пересекается с аргументами. При PENDING request владеет fid до
 * чтения статуса или закрытия. При отказе request равен NULL. */
StyxRpcLaunchStatus styxrpc_launch_named(struct StyxClient *client, const char *name,
    const StyxRpcProgram *program, const void *arguments, size_t size,
    void *scratch, size_t capacity, struct StyxFid **request);


/* EN: Read status from the same request fid; repeated reads at offset zero are allowed.
 * RU: Прочитать статус через fid запроса; допустимо повторять чтение со смещения ноль. */
StyxRpcLaunchStatus styxrpc_launch_named_status(struct StyxFid *request);


/* EN: Consume and clear the fid. Clunk requests cancellation if it is still running.
 * RU: Поглотить и очистить fid. Clunk запрашивает отмену незавершённого запуска. */
void styxrpc_launch_named_close(struct StyxFid **request);

#ifdef __cplusplus
}
#endif
#endif
