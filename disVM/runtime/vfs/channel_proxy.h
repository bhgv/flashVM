/* Copyright (c) 2026 bhgv
 * [GC:] EN: Local channel references and UniBus ops of a scoped Dis call.
 * RU: Локальные ссылки каналов и операции UniBus в пределах вызова Dis.
 */
#ifndef DVM_CHANNEL_PROXY_H
#define DVM_CHANNEL_PROXY_H
#include "channel_bus.h"
#include "cancel.h"
#include "returned_proxy.h"
#include "flashvm/srecord.h"

struct Channel; /* Dis heap object; never shared with the transport. */
typedef struct DVM_ChannelRoute DVM_ChannelRoute; /* Native immutable provider-side capability table. */


/* [GC:] EN: State owned by the active caller or its local decoder.
 * RU: Состояние принадлежит активному вызывающему или его декодировщику. */
typedef struct DVM_CallChannels
{
    struct Channel *objects[DVM_CHANNEL_BUS_SLOTS]; /* Owned and rooted until release. */
    DVM_ChannelSchema schemas[DVM_CHANNEL_BUS_SLOTS]; /* Exact exported or imported element types. */
    DVM_ChannelRoute *route; /* Owned decoding route until published to the call. */
    size_t count; /* Number of live entries. */
    int returning; /* Provider may encode only capabilities from the active call. */
    int resolving; /* Caller resolves reply keys to already exported originals. */
    DVM_Cancel *message; /* Borrowed exact provider scope while moving channel values. */
    DVM_ReturnedImport *imports; /* Borrowed caller-owned persistent result staging. */
} DVM_CallChannels;


/* EN: Release all channel roots on the same admitted Dis resident.
 * RU: Снять все корни каналов на том же допущенном исполнителе Dis. */
void dvm_call_channels_release(DVM_CallChannels *channels);


/* [GC:] EN: Drop unpublished caller capabilities from first through the live tail.
 * RU: Освободить неопубликованные полномочия клиента от first до конца таблицы. */
void dvm_call_channels_truncate(DVM_CallChannels *channels, size_t first);

/* EN: Encode a borrowed channel of the declared type or nil. Retain each exported original.
 * RU: Кодировать заимствованный канал объявленного типа либо nil. Удержать каждый исходный канал. */
int dvm_call_channel_encode(DVM_CallChannels *channels, const char *type, void *value,
    void *bytes, size_t capacity, size_t *size);


/* EN: Decode a channel capability into an owned local proxy, reusing aliases.
 * RU: Декодировать полномочие в собственный локальный прокси, повторяя ссылки. */
int dvm_call_channel_decode(DVM_CallChannels *channels, const char *type,
    const FVM_SRecord *record, void **result);


/* EN: Publish the prepared immutable native route before running the provider's Dis.
 * RU: Опубликовать готовый нативный маршрут до запуска Dis провайдера. */
int dvm_call_channels_install(DVM_CallChannels *channels);


/* EN: Retain or release a native route across inherited process environments.
 * RU: Удержать или освободить маршрут в наследуемых окружениях процессов. */
int dvm_channel_route_retain(DVM_ChannelRoute *route);
void dvm_channel_route_release(DVM_ChannelRoute *route);


/* EN: Revoke operations and clear the call owner's slot, leaving child pins valid.
 * RU: Отозвать операции и обнулить слот владельца, сохранив ссылки дочерних потоков. */
void dvm_channel_route_end(DVM_ChannelRoute **route);


/* EN: Test an already validated live Channel for the private proxy mover.
 * RU: Проверить, использует ли проверенный живой Channel прокси-обработчик. */
int dvm_call_channel_is_proxy(struct Channel *channel);


/* EN: Forward one blocking Dis operation. On receive, set the mapped destination.
 * RU: Передать блокирующую операцию Dis. При приёме записать mapped назначение. */
void dvm_call_channel_operate(struct Channel *channel, const void *cell, int receive, void *operand);


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


/* EN: Offer the first selected route's guards together for one source-clock window.
 * Return the original guard index, or fallback only after authoritative refusal.
 * RU: Предложить ветки первого выбранного маршрута на один интервал его часов.
 * Вернуть исходный индекс либо fallback только после ответа об отсутствии передачи. */
unsigned dvm_call_channel_alt_window(const DVM_ProxyGuard *guards, unsigned count, unsigned fallback);


/* EN: Drain one bounded round on the current caller, restoring GC wait mode.
 * RU: Выполнить один ограниченный проход у клиента, восстановив ожидание GC. */
int dvm_call_channel_progress(DVM_ChannelBus *bus, DVM_CallChannels *channels, int *native);

#endif
