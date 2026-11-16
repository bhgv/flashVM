/* Copyright (c) 2026 bhgv
 * EN: VFS-side event journal subscription and ordinary readonly resource constructor.
 * RU: VFS-side подписка на journal и конструктор обычного readonly ресурса.
 */
#ifndef STYX_EVENTS_PROVIDER_H
#define STYX_EVENTS_PROVIDER_H
#include "events.h"
#include "provider.h"


/* EN/RU: Retain borrowed source journal; adopt policy.state only on owned result success.
 * Explicit allow/encode required; initial current-state invalidation closes subscription race.
 * Host selects an authorized journal/view; this grants no publication/namespace authority.
 * Удержать journal; policy.state принять только при успехе. Host выбирает authorized view. */
StyxEvents *styxeventsource(DVM_Changes *source, const StyxEventPolicy *policy);


/* EN/RU: Consume one owned inbox ref on every outcome; owned ordinary provider/NULL.
 * Only root dot/r opens; aliases/opens share one publication inbox, not independent queues.
 * Publish through existing prepared authorized trees; no reserved global path is installed.
 * Принять inbox ref всегда; свой readonly provider/NULL. Opens разделяют одну очередь. */
DVM_Provider *styxeventprovider(StyxEvents *events);
#endif
