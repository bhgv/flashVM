/* Copyright (c) 2026 bhgv
 * EN: Native two-ended transmission of complete S-records and explicit delivery receipts.
 * RU: Нативная двусторонняя передача целых S-записей и явных подтверждений доставки.
 */
#ifndef DVM_VFS_RECORD_PIPE_H
#define DVM_VFS_RECORD_PIPE_H
#include "provider.h"

#define DVM_RECORD_PIPE_MAX 1024U /* Maximum complete wire record, including framing. */
#define DVM_RECORD_REJECTED 0U /* Receiver explicitly rejected the record. */
#define DVM_RECORD_DELIVERED 1U /* Receiver confirmed delivery. */
#define DVM_RECORD_UNKNOWN 2U /* Receiver closed after obtaining it, without a receipt. */


/* EN: Return an owned virtual directory with send and recv files, or NULL on
 * allocation failure. One transaction occupies the pipe until its sender reads
 * a receipt or closes. Neither provider operations nor close wait for a peer.
 * RU: Вернуть свой виртуальный каталог с файлами send и recv либо NULL при
 * ошибке выделения. Одна операция занимает канал до чтения подтверждения
 * отправителем или закрытия его FD. Операции и закрытие не ждут другую сторону. */
DVM_Provider *dvm_record_pipe(void);
#endif
