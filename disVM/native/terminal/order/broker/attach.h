/* Copyright (c) 2026 bhgv
 * EN: Explicit display-independent ordered Unibus attachment to captured launch channels.
 * RU: Явное подключение ordered Unibus к закреплённым каналам запуска независимо от дисплея.
 */
#ifndef STYX_ORDERED_ATTACH_H
#define STYX_ORDERED_ATTACH_H
#include "broker.h"
#include "launch/ticket.h"
#ifdef __cplusplus
extern "C" {
#endif


/* EN/RU: Borrowed native configuration through constructor return, never a serialized layout.
 * Native-конфигурация заимствуется до возврата конструктора, её layout не сериализуется.
 */
typedef struct StyxOrderedOptions
{
    size_t struct_size; /* Exact native descriptor size. */
    unsigned limit; /* Inner body bytes, including encoded tag,1..65472. */
    unsigned long long window; /* Per-tag future displacement1..2^63-1. */
    unsigned long lane; /* Explicit nonzero unsigned32 outgoing outer tag. */
    const unsigned long *tags; /* Unique authorized inner tags; NULL only with count0. */
    size_t count; /* Copied grant count;0 permits native services only. */
} StyxOrderedOptions;


/* EN: Validate only options,1/0, no I/O/allocation/workers. Every grant's encoded tag must fit limit.
 * Borrow options/tag array through return. Zero grants are valid for a native service-only owner.
 * RU: Проверить options:1/0, без I/O/alloc/workers. Кодированный tag каждого grant должен помещаться.
 * Options/tag array borrowed до возврата; ноль grants допускает только native-службы.
 */
int styxorderedoptionsvalid(const StyxOrderedOptions *options);


/* EN: Borrow client/channels/options; return owned broker/NULL without consuming caller references.
 * Validate all options and captured-channel syntax before opening fids. Preserve exact captured root/client
 * association through ordinary control construction; never ambient lookup, dialing or one-way downgrade.
 * Queue initial offer1; this is not completed negotiation or a delivery/execution acknowledgement.
 * On failure release only staged owners. Original session and unrelated fids stay with caller.
 * Current native cancellation scope applies to construction only; pre-cancel performs no I/O.
 * Cancellation/failure after admission cannot retract bytes already accepted; no replay or fallback.
 * Native owner retires all borrowed users before styxorderedfree. No display or VM is required.
 * RU: Client/channels/options borrowed; свой broker/NULL без поглощения исходных refs.
 * Проверить options и синтаксис каналов до открытия fid; сохранить captured root/client association.
 * Поставить offer1, не обещая завершённого согласования/доставки/исполнения. Без dial/ambient fallback.
 * Отказ освобождает лишь staged owners; исходный client и чужие fids остаются у caller. Дисплей/VM не нужны.
 */
StyxOrdered *styxbidirorderedchannels(StyxClient *client, const DVM_TerminalChannels *channels,
    const StyxOrderedOptions *options);


/* EN: Borrow an exclusive pre-adoption handoff/client/options; attach only its existing FlashVM channels.
 * Require live namespace and a channel-bearing description; absent display refuses without I/O.
 * No ticket take, namespace mutation, unibus binding, guest startup or FD transfer. Handoff stays unchanged
 * on every outcome and may be freed after success: returned control retains its own captured handles.
 * RU: Заимствовать exclusive handoff до adoption, client/options; подключить лишь его FlashVM-каналы.
 * Нужны namespace и descriptor каналов; отсутствие дисплея даёт отказ до I/O. Handoff не меняется.
 * Без take, изменения namespace, binding, guest-startup либо FD-transfer. После успеха handoff можно освободить.
 */
StyxOrdered *styxbidirorderedlaunch(StyxClient *client, const DVM_LaunchHandoff *handoff,
    const StyxOrderedOptions *options);
#ifdef __cplusplus
}
#endif
#endif
