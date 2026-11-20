/* Copyright (c) 2026 bhgv
 * EN: Owner-side admission for a path-only Styx launch endpoint.
 * RU: Приём запроса запуска по пути на стороне владельца Styx.
 */
#ifndef STYX_TERMINAL_LAUNCH_RECEIVE_H
#define STYX_TERMINAL_LAUNCH_RECEIVE_H
#include <stddef.h>
#include <stdint.h>


/* EN/RU: One-shot state owned by a serialized server endpoint.
 * Состояние одноразового узла с последовательными вызовами сервера. */
typedef struct StyxLaunchReceiver
{
    unsigned long tag;          /* Fixed host-configured S-record tag. */
    int (*start)(void *state);  /* Owner callback, returns 1 on admission and 0 on refusal. */
    void *state;               /* Borrowed by the callback, owned by the host. */
    int used;                  /* Initially zero; set before invoking start. */
} StyxLaunchReceiver;


/* EN: Accept one complete empty launch frame at offset zero. Return 1 only
 * when the owner callback accepts it. Malformed frames leave the receiver
 * available, while invoking the callback consumes it even on refusal.
 * The host serializes calls, controls who can write and pins receiver and bytes.
 * RU: Принять один полный пустой кадр запуска со смещением ноль. Вернуть 1
 * только при принятии владельцем. Некорректный кадр оставляет узел доступным,
 * вызов callback поглощает его даже при отказе. Хост сериализует вызовы,
 * управляет правом записи и удерживает receiver и bytes. */
int styxterminallaunchreceive(StyxLaunchReceiver *receiver, uint64_t offset,
    const void *bytes, size_t size);

#endif
