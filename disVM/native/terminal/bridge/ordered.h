/* Copyright (c) 2026 bhgv
 * EN: Universal value-channel terminal over negotiated Unibus, independent of IDE/application policy.
 * RU: Универсальный терминал value-каналов через согласованный Unibus, без IDE-политики приложения.
 */
#ifndef STYX_ORDERED_TERMINAL_H
#define STYX_ORDERED_TERMINAL_H
#include "bridge.h"
#include "order/broker/broker.h"
#ifdef __cplusplus
extern "C" {
#endif


/* EN: Native-only launch options, borrowed during constructor; never serialize pointers/native layout.
 * RU: Native-настройки запуска, заимствованы на время конструктора; pointers/native-раскладка не передаются.
 */
typedef struct StyxOrderedTerminalOptions
{
    size_t struct_size; /* Exact descriptor size; initialize with sizeof(options). */
    unsigned local_side; /* DVM_UI_DIS or DVM_UI_UI retained by the local host. */
    unsigned limit; /* Inner body ceiling; outer control reserves another64 bytes. */
    unsigned long long window; /* Agreed future ordinal displacement ceiling,1..2^63-1. */
    unsigned long lane; /* Independently selected nonzero outgoing outer wrapper tag. */
    unsigned long ui_tag; /* Predeclared inner UI tag, zero allowed and remains application-local. */
    const unsigned long *tags; /* Borrowed list of unique UI/resource/other inner grants. */
    size_t count; /* List count; list must include ui_tag, copied by native broker before publication. */
} StyxOrderedTerminalOptions;


/* EN: Consume peer on every outcome; own the existing local value channels and two native relays.
 * UI tag must already be granted and local ceiling fit every valid fvd1 frame. Unknown services and
 * other client tags remain native host responsibility through the borrowed peer accessor.
 * Negotiation may finish later; TX relay waits for its profile/counter, never drives a guest.
 * RU: Поглотить peer при любом исходе; создать стандартные локальные value-каналы и два native relays.
 * UI-тег уже разрешён, ceiling вмещает fvd1. Согласование может завершиться позже; relay ждёт лишь свой TX.
 */
StyxBidirUIBridge *styxbidirordereduibridge(StyxOrdered *peer, unsigned local_side, unsigned long ui_tag);


/* EN: Borrow the same captured launch client/channels/options; construct control, broker, local bridge and queue offer1.
 * Return owned bridge/NULL. Never consumes caller's client/provider, never ambient path fallback or one-way downgrade.
 * Caller selects this explicit launch profile; an absent display simply need not construct this adapter.
 * RU: Заимствовать исходный client/channels/options; создать control/брокер/мост и поставить offer1.
 * Свой bridge/NULL; исходные client/provider не поглощаются, фонового поиска и one-way downgrade нет.
 */
StyxBidirUIBridge *styxbidirorderedterminal(StyxClient *client, const DVM_TerminalChannels *channels,
    const StyxOrderedTerminalOptions *options);


/* EN: Borrow owned negotiated peer for other clients/services, never consume the UI inbox separately.
 * NULL for legacy flat bridge. Lifetime through bridge free, no ownership transfer.
 * RU: Заимствовать peer для других клиентов/служб, UI-очередь отдельно не читать. NULL у прежнего flat-моста.
 */
StyxOrdered *styxbidiruibridgeordered(StyxBidirUIBridge *bridge);
#ifdef __cplusplus
}
#endif
#endif
