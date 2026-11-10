/* Copyright (c) 2026 bhgv
 * EN: Lifecycle of native execution threads permanently assigned to one Prog.
 * RU: Жизненный цикл нативных потоков, закреплённых за одним Prog.
 */
#ifndef DVM_MANAGED_WORKERS_H
#define DVM_MANAGED_WORKERS_H



enum
{
    DVM_MANAGED_TURN_IDLE,
    DVM_MANAGED_TURN_GUEST,
    DVM_MANAGED_TURN_MAINTENANCE
};


/* EN: Start the resident registry on the active controller. count must be one,
 * automatic and limit zero. Other values fail with a removed-pool diagnostic.
 * turn and cancel_waits are borrowed callbacks until stop. diagnostics enables
 * lifecycle output. Return zero on invalid state or native allocation failure.
 * RU: Запустить реестр закреплённых потоков на активном контроллере.
 * count равен одному, automatic и limit нулю. Другие значения отклоняются.
 * Обратные вызовы заимствованы до stop. При ошибке вернуть ноль. */
int dvm_managed_workers_start(int count, int automatic, unsigned long limit, int diagnostics,
    int (*turn)(void), void (*cancel_waits)(void));


/* EN: Start and retire one resident on its permanently assigned thread.
 * RU: Запуск и завершение процесса на закреплённом потоке. */
struct Prog;
int dvm_managed_workers_start_guest(struct Prog *guest);
int dvm_runtime_prepare_resident(struct Prog *guest);
struct Prog *dvm_managed_workers_guest(void);
void dvm_managed_workers_guest_retired(int pid);

/* EN: Query a staged native reply after inspecting the resident's FIFO.
 * RU: Проверить извлечённый ответ после проверки FIFO процесса. */
int dvm_managed_workers_reply_in_transit(struct Prog *guest);

/* EN: Take the current owner's staged reply, if any.
 * RU: Забрать извлечённый ответ текущего владельца, если он есть. */
struct DVM_OwnerMessage *dvm_managed_workers_take_reply(struct Prog *guest);
void dvm_managed_workers_finish_reply(struct Prog *guest);


/* EN: Retained native-wait hook. Resident threads need no replacement
 * executor. Returns NULL without creating threads or changing ownership.
 * RU: Совместимый вход ожидания. Вернуть NULL без создания исполнителей. */
const char *dvm_managed_workers_native_wait(void);


/* EN: Compatibility hook for native completion accounting. arriving is ignored,
 * return zero. Resident identity remains assigned throughout the native call.
 * RU: Совместимый вход завершения native. arriving игнорируется, вернуть ноль. */
int dvm_managed_workers_returning(int arriving);


/* EN: Compatibility notification of native suspension. No state is changed.
 * RU: Совместимое уведомление об ожидании native. Состояние не меняется. */
void dvm_managed_workers_native_suspended(void);


/* Signal runnable work or a pending native request. The caller must own
 * NativeWorld or otherwise guarantee service lifetime through the signal. */
void dvm_managed_workers_wake(void);
void dvm_managed_workers_wake_guest(struct Prog *guest);


/* Yield an empty controller turn through FIFO heap admission. May wait for
 * another executor or collector. Return zero for a worker/protocol failure.
 * idle requests a bounded native sleep OUTSIDE the heap before reentry.
 * No current Prog, native token or C-held managed root may be live here. */
int dvm_managed_workers_yield(int idle);


/* Park a rootless non-UI controller awaiting logical channel work. Heap is
 * released; pending checks native requests after the wake epoch is captured.
 * The callback is invoked without the scheduler mutex. */
int dvm_managed_workers_park(int (*pending)(void));


/* Count one controller scheduler turn, only when the optional service is live. */
void dvm_managed_workers_record(void);


/* Count one ordinary file operation on the ACTIVE heap owner. No handle/root is
 * retained; the separate per-handle FIFO serializes native cursor access. */
void dvm_managed_workers_file_call(void);


/* Authorize only the live helper's ACTIVE claimed turn for copied UI RPC.
 * Never authorizes UI main, foreign TLS or a native waiter; no root is returned.
 * Requires the caller's current phase check before reading shared heap identity. */
int dvm_managed_workers_current(void);


/* EN: Stop and join helpers, leaving the controller idle for collector shutdown.
 * RU: Остановить и присоединить исполнителей, оставив контроллер в IDLE.
 * diagnostics reports joined counters. No-op if the service was never started. */
int dvm_managed_workers_stop(int diagnostics);
#endif
