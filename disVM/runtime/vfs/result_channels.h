/* Copyright (c) 2026 bhgv
 * EN: Host returned Dis channels on a dedicated provider resident.
 * RU: Обслуживание возвращаемых каналов Dis выделенным процессом провайдера.
 */
#ifndef DVM_RESULT_CHANNELS_H
#define DVM_RESULT_CHANNELS_H
#include "channel_proxy.h"

/* EN: Copy the active mounted function's native publication identity, if available.
 * RU: Скопировать идентификатор нативной публикации активного вызова при наличии. */
int dvm_result_channels_token(unsigned char token[STYX_CHANNEL_EXPORT_TOKEN_SIZE]);


/* [GC:] EN: Publish the staged channels and transfer roots to a dedicated resident.
 * Consumes the staged channels on success or leaves the caller's table releasable on failure.
 * RU: Опубликовать подготовленные каналы и передать корни выделенному процессу.
 * При успехе поглощает каналы, при отказе таблицу вызывающего можно освободить. */
int dvm_result_channels_finish(DVM_CallChannels *channels);

/* EN: Report an external channel pump from launch reservation through retirement.
 * RU: Проверить наличие сетевой службы с резервирования запуска до завершения. */
int dvm_result_channels_active(void);


/* EN: Borrowed native transport routes to retain for a message service.
 * RU: Заимствованные нативные маршруты для удержания службой сообщений. */
typedef struct DVM_ChannelTransport
{
    DVM_Provider *reverse; /* Reverse session root for PID grants. */
    DVM_Provider *forward; /* Mounted UniBus for module token grants. */
    DVM_Provider *control; /* Provider registry anchor, revoked when owned by this service. */
    int revoke; /* Revoke a derived control at service retirement. */
} DVM_ChannelTransport;


/* [GC:] EN: Start a native bus pump with retained provider and cancellation.
 * On success consumes channels; on failure leaves the table releasable.
 * RU: Запустить службу с удержанием провайдера и отмены. При успехе
 * принимает channels, при отказе таблицу можно освободить. */
int dvm_result_channels_start(DVM_CallChannels *channels, DVM_ChannelBus *bus,
    DVM_Provider *provider, DVM_Cancel *cancel, const DVM_ChannelTransport *transport);
#endif
