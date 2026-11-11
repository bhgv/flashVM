/* Copyright (c) 2026 bhgv
 * EN: Borrowed per-channel matching reservations under the channel queue mutex.
 * RU: Резервации сопоставления по каналам под мьютексом очередей каналов.
 */
#ifndef DVM_CHANNEL_MATCH_H
#define DVM_CHANNEL_MATCH_H


/* EN: Caller-owned stack ledger, live through transfer and exception cleanup.
 * RU: Стековая запись вызывающего, живая до передачи и очистки исключения. */
typedef struct DVM_ChannelMatch
{
    struct DVM_ChannelMatch *next; /* Protected by the caller's channel mutex. */
    void *channel; /* Borrowed channel identity, never dereferenced here. */
    void *owner; /* Native owner identity, never dereferenced here. */
} DVM_ChannelMatch;


/* EN: Reserve channel for owner in zeroed match under the queue mutex; zero if
 * already reserved. Caller preserves all lifetimes until end; no allocation.
 * RU: Зарезервировать channel за owner в нулевой match под мьютексом очереди;
 * ноль, если уже занят. Вызывающий хранит объекты до end. Выделения памяти нет. */
int dvm_channel_match_begin(DVM_ChannelMatch *match, void *channel, void *owner);


/* EN: Under the queue mutex, test whether another owner holds channel.
 * Own nested buffer reads/refills remain permitted. Borrowed identity inputs.
 * RU: Под мьютексом проверить, держит ли channel другой owner. Свои вложенные
 * чтение/пополнение разрешены. Аргументы — заимствованные идентификаторы. */
int dvm_channel_match_blocked(void *channel, void *owner);


/* EN: Unlink exact match under the queue mutex; invalid identity aborts.
 * Caller then notifies waiters and can release borrowed channel/owner lifetimes.
 * RU: Удалить точную match под мьютексом; неверная запись завершает процесс.
 * Затем вызывающий уведомляет ожидающих и освобождает заимствованные объекты. */
void dvm_channel_match_end(DVM_ChannelMatch *match);

#endif
