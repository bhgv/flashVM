/* Copyright (c) 2026 bhgv
 * EN: Native pre-init namespace preparation from an already attached terminal Styx session.
 * RU: Native подготовка namespace перед init из уже подключённой Styx-сессии терминала.
 */
#ifndef STYX_LAUNCH_PREPARE_H
#define STYX_LAUNCH_PREPARE_H
#include "client.h"
#include "launch/prepare.h"


/* EN: Consume one terminal client ref on every outcome; owned isolated namespace/NULL.
 * options borrowed through return: terminal MUST be NULL, provider is an already
 * filtered live grant, origins/policy are trusted copied launch associations.
 * access is a fixed local ceiling, must contain READ|EXEC; optional WRITE does
 * not grant remote permission. Capture the existing attached root, build the
 * usual terminal-first union, and publish only after successful validation.
 * Native I/O may wait; current cancellation scope is respected. Caller retains
 * provider/options through return. No dialing, authentication, module execution,
 * reverse-session creation or physical link ownership is added. An ordinary
 * session or an explicitly configured bidir channel works identically here.
 * RU: Поглотить terminal ref всегда; свой namespace/NULL. options borrowed,
 * terminal обязан быть NULL, provider — готовый filtered grant. access ceiling
 * содержит READ|EXEC, WRITE optional и не выдаёт прав. Capture attached root,
 * обычный terminal-first union до init. Native I/O с cancel scope, без dial/auth,
 * запуска guest, создания bidir или владения physical link. */
DVM_Namespace *styxlaunchprepare(StyxClient *terminal, const DVM_LaunchOptions *options, unsigned access);
#endif
