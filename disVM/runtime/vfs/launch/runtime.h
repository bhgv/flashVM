/* Copyright (c) 2026 bhgv
 * EN: Trusted pre-publication launch adoption; no running/foreign guest control API.
 * RU: Trusted принятие запуска до публикации; не API управления работающим/чужим guest.
 */
#ifndef DVM_LAUNCH_RUNTIME_H
#define DVM_LAUNCH_RUNTIME_H
#include "ticket.h"
struct Prog; /* Runtime-owned unpublished process, borrowed by its constructor. */
struct Module; /* Loaded provider-local module, managed by the runtime controller. */


/* EN: Run one DisVM invocation on the host launch thread, with the usual CLI
 * options and a host-selected local bootstrap path. Borrow argv and the exclusive
 * optional handoff through return. Successful adoption transfers handoff grants
 * to the initial guest. The caller frees the shell after runtime shutdown.
 * Returns the CLI exit status. One invocation per process; no concurrent calls.
 * RU: Выполнить один запуск DisVM в стартовом потоке с обычными CLI-опциями
 * и локальным bootstrap, выбранным host. Argv и необязательный exclusive handoff
 * заимствованы до возврата. При adoption grants переходят первому гостю,
 * вызывающий освобождает оболочку после завершения runtime. Возвращает CLI status.
 * Один вызов на процесс, без конкурентных вызовов. */
int dvm_runtime_main_with_handoff(int argc, char **argv, DVM_LaunchHandoff *handoff);
/* EN/RU: Boot the embedded listener in the supplied owner namespace. / Запустить встроенный приёмник. */
int dvm_runtime_main_launch_listener(int argc, char **argv, DVM_LaunchHandoff *handoff);


/* EN/RU: Controller starts an already loaded zero-argument bootstrap with optional handoff.
 * Requires ACTIVE startup controller, before guest dispatch; returns published initial process.
 * Adoption moves group/channel ownership and copies immutable startup label/bytes.
 * Later setup exceptions do not restore that ownership.
 * Caller owns/frees handoff shell and remaining metadata on success/error; NULL keeps ordinary boot.
 * Module is borrowed; caller retains/releases its original reference on both success and exception.
 * Controller запускает loaded bootstrap до dispatch; handoff ownership переносится при adoption,
 * поздние exceptions не возвращают group/channels; caller освобождает handoff shell/metadata. */
struct Prog *dvm_runtime_launch_module(struct Module *module, DVM_LaunchHandoff *handoff);

/* EN: Start a hosted child image under the active guest with an owned handoff.
 * RU: Запустить дочерний образ у активного гостя с передачей окружения. */
typedef struct DVM_LaunchCompletion
{
    void (*report)(void *, int); /* Native callback receiving zero on success. */
    void *state; /* Borrowed callback target through child retirement. */
    void (*started)(void *, int); /* Optional notification before the first instruction. */
} DVM_LaunchCompletion;

/* EN/RU: Stop the registered child matching the native launch identity. / Остановить потомка данного запуска. */
void dvm_runtime_cancel_hosted_launch(int pid, void *state);

struct Prog *dvm_runtime_launch_child(struct Module *module, DVM_LaunchHandoff *handoff,
    const DVM_LaunchCompletion *completion);


/* EN/RU: Adopt once into initialized unpublished process;1 success/0 invalid/OOM, no guest execution.
 * Stage namespace, channels and startup data before mutation. Explicit numbered files
 * can block in native opens. Failure leaves the process and handoff unchanged and closes
 * files already acquired, which may have consumed their one-use grants.
 * Caller exclusively owns fresh process/handoff before any guest dispatch.
 * Принять один раз до dispatch;1/0, process/handoff exclusive. Открытие номерных
 * файлов может ждать. При отказе process/handoff сохраняются, открытые файлы закрываются,
 * уже полученные одноразовые выдачи остаются поглощёнными. */
int dvm_launch_process_adopt(struct Prog *process, DVM_LaunchHandoff *handoff);


/* EN/RU: Retain inherited immutable startup data, copy channel descriptor and prepare child movie service;1/0, native only.
 * Fresh child/borrowed parent are constructor-owned; failure leaves child empty.
 * Удержать startup data, скопировать inherited descriptor с provider pin;1/0, fresh child/parent принадлежат constructor. */
int dvm_launch_process_inherit(struct Prog *process, struct Prog *parent);


/* EN/RU: Retire a process's startup-data pin and channel descriptor after its owner/users stop; NULL allowed.
 * Освободить startup-data pin и descriptor после завершения owner/users; NULL допустим, без managed access. */
void dvm_launch_process_release(struct Prog *process);


/* EN/RU: Constructor-only final announcement commit;1/0, native notices only, no I/O.
 * Commit объявления только в constructor;1/0, native notices без I/O. */
int dvm_launch_process_announce(struct Prog *process);

#endif
