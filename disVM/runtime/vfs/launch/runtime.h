/* Copyright (c) 2026 bhgv
 * EN: Trusted pre-publication launch adoption; no running/foreign guest control API.
 * RU: Trusted принятие запуска до публикации; не API управления работающим/чужим guest.
 */
#ifndef DVM_LAUNCH_RUNTIME_H
#define DVM_LAUNCH_RUNTIME_H
#include "ticket.h"
struct Prog; /* Runtime-owned unpublished process, borrowed by its constructor. */
struct Module; /* Loaded provider-local module, managed by the runtime controller. */


/* EN/RU: Controller starts an already loaded zero-argument bootstrap with optional handoff.
 * Requires ACTIVE startup controller, before guest dispatch; returns published initial process.
 * Adoption moves group/channel ownership; later setup exceptions do not restore that ownership.
 * Caller owns/frees handoff shell and remaining metadata on success/error; NULL keeps ordinary boot.
 * Module is borrowed; caller retains/releases its original reference on both success and exception.
 * Controller запускает loaded bootstrap до dispatch; handoff ownership переносится при adoption,
 * поздние exceptions не возвращают group/channels; caller освобождает handoff shell/metadata. */
struct Prog *dvm_runtime_launch_module(struct Module *module, DVM_LaunchHandoff *handoff);


/* EN/RU: Adopt once into initialized unpublished process;1 success/0 invalid/OOM, no guest execution.
 * Validate/stage before mutation; on failure leave process and handoff unchanged.
 * Caller exclusively owns fresh process/handoff; no running owner, messages or native wait allowed.
 * Принять один раз в private process;1/0, до commit ничего не меняется, process/handoff exclusive. */
int dvm_launch_process_adopt(struct Prog *process, DVM_LaunchHandoff *handoff);


/* EN/RU: Copy inherited channel descriptor and prepare child movie service;1/0, native allocation only.
 * Fresh child/borrowed parent are constructor-owned; failure leaves child empty.
 * Скопировать inherited descriptor с provider pin;1/0, fresh child/parent принадлежат constructor. */
int dvm_launch_process_inherit(struct Prog *process, struct Prog *parent);


/* EN/RU: Retire a process's channel descriptor after its owner/users stop; NULL process allowed.
 * Освободить descriptor после завершения owner/users; NULL допустим, без managed access. */
void dvm_launch_process_release(struct Prog *process);
#endif
