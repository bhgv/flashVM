/* Copyright (c) 2026 bhgv
 * [GC:] EN: Local channel references and UniBus ops of a scoped Dis call.
 * RU: Локальные ссылки каналов и операции UniBus в пределах вызова Dis.
 */
#ifndef DVM_CHANNEL_PROXY_H
#define DVM_CHANNEL_PROXY_H
#include "channel_bus.h"
#include "flashvm/srecord.h"

struct Channel; /* Dis heap object; never shared with the transport. */


/* [GC:] EN: State owned by the active caller or its local decoder.
 * RU: Состояние принадлежит активному вызывающему или его декодировщику. */
typedef struct DVM_CallChannels
{
    struct Channel *objects[DVM_CHANNEL_BUS_SLOTS]; /* Owned and rooted until release. */
    uint32_t keys[DVM_CHANNEL_BUS_SLOTS]; /* Exact exported or imported capabilities. */
    size_t count; /* Number of live entries. */
} DVM_CallChannels;


/* EN: Release all channel roots on the same admitted Dis resident.
 * RU: Снять все корни каналов на том же допущенном исполнителе Dis. */
void dvm_call_channels_release(DVM_CallChannels *channels);


/* EN: Encode a borrowed native chan of int or nil. Retain each new identity.
 * RU: Кодировать заимствованный chan of int либо nil. Удержать новый объект. */
int dvm_call_channel_encode(DVM_CallChannels *channels, void *value,
    void *bytes, size_t capacity, size_t *size);


/* EN: Decode a channel capability into an owned local proxy, reusing aliases.
 * RU: Декодировать полномочие в собственный локальный прокси, повторяя ссылки. */
int dvm_call_channel_decode(DVM_CallChannels *channels, const FVM_SRecord *record, void **result);


/* EN: Test an already validated live Channel for the private proxy mover.
 * RU: Проверить, использует ли проверенный живой Channel прокси-обработчик. */
int dvm_call_channel_is_proxy(struct Channel *channel);


/* EN: Forward one blocking Dis operation. On receive, set the mapped destination.
 * RU: Передать блокирующую операцию Dis. При приёме записать mapped назначение. */
void dvm_call_channel_operate(struct Channel *channel, int receive, void *destination, int64_t value);


/* EN: One already captured guard of a remote selection.
 * RU: Одна уже захваченная ветка удалённой операции выбора. */
typedef struct DVM_ProxyGuard
{
    struct Channel *channel; /* Borrowed, live proxy from the active frame. */
    void *operand; /* Mapped receive destination or borrowed send value. */
    unsigned index; /* Original Dis alt arm number. */
    int receive; /* True for receive. */
} DVM_ProxyGuard;


/* EN: Select one guard over UniBus. Return its original index or the fallback.
 * RU: Выбрать одну ветку через UniBus. Вернуть её индекс или запасную ветку. */
unsigned dvm_call_channel_alt(const DVM_ProxyGuard *guards, unsigned count, int block);


/* EN: Drain one bounded round on the current caller, restoring GC wait mode.
 * RU: Выполнить один ограниченный проход у клиента, восстановив ожидание GC. */
int dvm_call_channel_progress(DVM_ChannelBus *bus, const DVM_CallChannels *channels, int *native);

#endif
