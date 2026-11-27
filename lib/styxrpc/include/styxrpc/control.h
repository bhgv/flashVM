/* Copyright (c) 2026 bhgv
 * EN: Owner callbacks exposed as counted upload and text state files.
 * RU: Колбэки владельца в файлах загрузки с длиной и текстового состояния.
 */
#ifndef STYXRPC_CONTROL_H
#define STYXRPC_CONTROL_H
#include "styxrpc/serve.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_UPLOAD_PREFIX 4U /* Unsigned big-endian payload length in the write stream. */
#define STYXRPC_STATE_LIMIT 4096U /* Maximum bytes in an immutable per-fid status snapshot. */

typedef enum StyxRpcControlCommand
{
    STYXRPC_CONTROL_START,   /* Start or resume the current image. */
    STYXRPC_CONTROL_PAUSE,   /* Pause execution at the owner boundary. */
    STYXRPC_CONTROL_RESTART, /* Replace current state with a fresh execution of the same image. */
    STYXRPC_CONTROL_DEBUG    /* Enter the owner's debug mode. */
} StyxRpcControlCommand;

typedef struct StyxRpcControlSpec
{
    void *context;             /* Owner state borrowed until every server is retired. */
    uint32_t upload_limit;     /* Maximum byte length of one payload. */
    int (*upload)(void *, const void *, size_t);                  /* Borrow complete payload and return 1 on commit. */
    int (*command)(void *, StyxRpcControlCommand);                /* Apply one exact recognized word. */
    int (*status)(void *, char *, size_t, size_t *);              /* Copy a text snapshot, returning 1 on success. */
} StyxRpcControlSpec;

typedef struct StyxRpcControl StyxRpcControl; /* Owns descriptor and serializes a single staged upload. */


/* EN: Copy the callbacks into an owner-local control tree, preserving out on failure.
 * RU: Скопировать колбэки в дерево владельца, сохраняя out при ошибке. */
StyxRpcStatus styxrpc_control_create(const StyxRpcControlSpec *spec, StyxRpcControl **out);


/* EN: Destroy only after all using servers have closed their fids. NULL is accepted.
 * RU: Освободить после закрытия fid во всех зависимых серверах. Допускается NULL. */
void styxrpc_control_destroy(StyxRpcControl *control);

/* EN: Handle an additional counted .state command after the four standard words.
 * Set on the server owner. The callback borrows the original spec context and
 * the bytes until it returns. Return 1 to accept, otherwise reject. NULL clears.
 * RU: Обработать дополнительную команду .state после четырёх стандартных слов.
 * Установить в потоке владельца. Колбэк заимствует context и байты до возврата.
 * Возвратить 1 при принятии команды. NULL отключает обработчик. */
void styxrpc_control_set_extension(StyxRpcControl *control, int (*command)(void *, const char *, size_t));

/* EN: Wait for one owner server cycle with only .state file I/O enabled.
 * Use between ordinary steps on a control or module-slot tree. Return 0 on
 * disconnect, error or recursive use of the same server. Fid cleanup still runs.
 * RU: Дождаться цикла владельца с файловым вводом-выводом только для .state.
 * Вызов между обычными шагами дерева управления или модуля. При обрыве,
 * ошибке или рекурсии вернуть 0. Очистка fid по-прежнему выполняется. */
int styxrpc_server_step_control(StyxRpcServer *server);

extern const StyxRpcTree styxrpc_control_tree; /* Publish .upload and .state directly at the root. */

#ifdef __cplusplus
}
#endif
#endif
