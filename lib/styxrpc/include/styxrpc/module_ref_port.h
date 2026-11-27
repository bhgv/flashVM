/* Copyright (c) 2026 bhgv
 * EN: Expose one scoped module reference as a positioned byte-oriented handle.
 * RU: Представление одной контекстной ссылки на модуль через позиционный байтовый дескриптор.
 */
#ifndef STYXRPC_MODULE_REF_PORT_H
#define STYXRPC_MODULE_REF_PORT_H
#include "styxrpc/module_ref.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_REF_PORT_CAPTURE 1U /* First write: opcode, writable byte, canonical path. */
#define STYXRPC_REF_PORT_ADOPT 2U   /* First write: opcode, then the complete named frame. */
#define STYXRPC_REF_PORT_FRAME 4U   /* Read offset of the complete tagged descriptor. */

typedef struct StyxRpcModuleRefPort StyxRpcModuleRefPort; /* Retains the remote mount and owns any opened file. */

/* EN: Open a local export with the descriptor's exact path, Qid and permissions.
 * Write an owned file to the zeroed out on OK. Leave it empty on failure.
 * RU: Открыть местный экспорт по точным пути, Qid и правам дескриптора.
 * При OK передать свой файл в обнулённый out. При отказе оставить out пустым. */
typedef StyxRpcIoOutcome (*StyxRpcModuleRefLocalOpen)(void *context, const StyxRpcModuleRef *ref,
    StyxRpcFile *out);


/* EN: Pin issuer and copy the connection scope for the selected owner. Return an unbound port.
 * RU: Удержать подключение issuer и скопировать область для выбранного владельца. Вернуть пустой порт. */
StyxRpcModuleRefPort *styxrpc_module_ref_port_open(struct StyxClient *issuer, unsigned owner, const void *scope);


/* EN: Also accept the opposite, local owner. Borrow resolver state through port close.
 * A NULL local callback limits adoption to the ordinary issuer.
 * RU: Принимать также встречного местного владельца. Заимствовать контекст до закрытия порта.
 * Колбэк local, равный NULL, ограничивает приём обычным удалённым владельцем. */
StyxRpcModuleRefPort *styxrpc_module_ref_port_open_paired(struct StyxClient *issuer, unsigned owner,
    const void *scope, StyxRpcModuleRefLocalOpen local, void *context);


/* EN: Read four value bytes at zero, or a descriptor fragment at FRAME and above.
 * Serialize use of port. Value I/O can wait for its issuer. Confirm the byte count in used.
 * RU: Прочитать четыре байта значения с нуля или фрагмент дескриптора с FRAME и далее.
 * Использовать порт последовательно. Чтение значения может ждать владельца. used получает число байтов. */
StyxRpcIoOutcome styxrpc_module_ref_port_read(StyxRpcModuleRefPort *port, uint64_t offset,
    void *bytes, size_t size, size_t *used);


/* EN: Bind with one complete offset-zero command, or replace four bytes on the bound port.
 * ADOPT validates the frame without opening a remote fid. CAPTURE opens and keeps one.
 * RU: Привязать полной командой с нуля либо заменить четыре байта значения привязанного порта.
 * ADOPT проверяет кадр без открытия удалённого fid. CAPTURE открывает и удерживает fid. */
StyxRpcIoOutcome styxrpc_module_ref_port_write(StyxRpcModuleRefPort *port, uint64_t offset,
    const void *bytes, size_t size);


/* EN: Consume port. Remote clunk is deferred, local close follows its VFS contract. NULL is allowed.
 * RU: Освободить порт. Удалённый clunk отложен, местный close следует контракту VFS. NULL допустим. */
void styxrpc_module_ref_port_close(StyxRpcModuleRefPort *port);

#ifdef __cplusplus
}
#endif
#endif
