/* Copyright (c) 2026 bhgv
 * [GC:] EN: Real-thread borrow admission before and after world sealing.
 * RU: Проверка допуска borrow до и после sealing мира на настоящих потоках.
 */
#include "lib9.h"
#include "dat.h"
#include "interp.h"
#include "disvm/native_process.h"
#include "disvm/native_world.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* [GC:] EN: C++98 consumer exercises the public C ABI before any sealing.
 * world/participant are borrowed and idle; return one after a matched borrow.
 * RU: Клиент C++98 проверяет публичный C ABI до sealing. world/participant
 * заимствованы и idle; один после парного borrow без передачи владения. */
extern int dvm_test_cpp_epochs(DVM_NativeWorld *world, DVM_WorldParticipant *participant);


/* [GC:] EN: Main owns storage through service join; sync orders all stages.
 * RU: Main владеет памятью до join службы; sync упорядочивает все стадии. */
typedef struct SealProbe
{
    DVM_NativeWorld *world;    /* Live through both participant detaches. */
    DVM_NativeContext *reader; /* Bound only to the actual service thread. */
    FVM_PortSync *sync;        /* Fixture rendezvous, never a heap exclusion. */
    unsigned int stage;        /* Monotonic protocol stage, protected by sync. */
} SealProbe;


/* [GC:] EN: Reject false condition with borrowed message and exact status 12.
 * No recovery or ownership transfer; failures never count as positive evidence.
 * RU: При ложном condition вывести message и код 12. Без восстановления
 * или передачи владения; отказ не считается положительным подтверждением. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL sealed borrow: %s\n", message);
        exit(12);
    }
}


/* [GC:] EN: Publish stage on borrowed probe under sync and wake the peer.
 * No managed state changes; storage survives join. No return/failure recovery.
 * RU: Под sync опубликовать stage в probe и разбудить соседа. Управляемое
 * состояние не меняется; память живёт до join. Без возврата и восстановления. */
static void advance(SealProbe *probe, unsigned int stage)
{
    fvm_port_sync_lock(probe->sync);
    require(probe->stage < stage, "ordered stage publication");
    probe->stage = stage;
    fvm_port_sync_signal_all(probe->sync);
    fvm_port_sync_unlock(probe->sync);
}


/* [GC:] EN: Wait for stage in borrowed probe; release sync while sleeping.
 * Caller holds no heap guards. No managed access or ownership change.
 * RU: Дождаться stage в probe, освобождая sync при ожидании. Вызывающий
 * не держит heap guards. Без доступа к управляемым данным и смены владения. */
static void await_stage(SealProbe *probe, unsigned int stage)
{
    fvm_port_sync_lock(probe->sync);
    while (probe->stage < stage)
    {
        fvm_port_sync_wait(probe->sync);
    }
    fvm_port_sync_unlock(probe->sync);
}


/* [GC:] EN: Bind the service from borrowed probe, hold two nested borrows,
 * then remain registered while main seals. New borrow must refuse unchanged.
 * Detach before TLS unbind; main owns context/storage until actual join.
 * RU: Привязать службу из probe и удержать два вложенных borrow, затем
 * сохранить регистрацию на время sealing. Новый borrow обязан отказать.
 * Снять регистрацию до TLS; main владеет контекстом и памятью до join. */
static void service(void *argument)
{
    SealProbe *probe = argument;
    require(dvm_native_context_bind(probe->reader), "bind real service");
    DVM_WorldParticipant *participant = dvm_world_join_service(probe->world, probe->reader);
    require(participant && dvm_world_read_begin(participant) && dvm_world_read_begin(participant),
            "nested service borrow before sealing");
    advance(probe, 1);
    await_stage(probe, 2);
    require(dvm_world_read_end(participant), "end inner service borrow");
    advance(probe, 3);
    await_stage(probe, 4);
    require(dvm_world_read_end(participant), "end final service borrow");
    advance(probe, 5);
    await_stage(probe, 6);
    require(!dvm_world_read_begin(participant), "service borrow reopened sealed world");
    require(!dvm_world_has_borrowers(probe->world) && !dvm_world_read_end(participant),
            "refusal leaves no phantom borrow");
    require(!dvm_world_try_enter_shared(participant), "sealed shared admission remains closed");
    require(dvm_world_detach(participant) && dvm_native_context_unbind(probe->reader), "service detach and TLS unbind");
}


/* [GC:] EN: argv selects locked polling, concurrent policy and idle controller.
 * Own worlds, contexts and one OS service; verify nested borrow cutoffs and
 * exact refusal before state mutation, then join and destroy every resource.
 * Return zero only on success; false invariants terminate with status 12.
 * RU: argv выбирает locked-опрос, конкурентную политику и idle-контроллер.
 * Владеть мирами, контекстами и OS-службой; проверить вложенные borrow и
 * отказы до изменения состояния, выполнить join и удалить ресурсы. Ноль —
 * успех; неверный инвариант завершает процесс кодом 12. */
int main(int argc, char **argv)
{
    require(argc == 4, "explicit fixture modes");
    int locked = strcmp(argv[1], "locked") == 0;
    int concurrent = strcmp(argv[2], "concurrent") == 0;
    int idle = strcmp(argv[3], "idle") == 0;
    SealProbe probe = {0};
    probe.world = concurrent ? dvm_world_create_concurrent(locked) : dvm_world_create_serial(locked);
    probe.sync = fvm_port_sync_create();
    DVM_NativeContext *controller = dvm_native_process_create(NULL);
    probe.reader = dvm_native_process_create(NULL);
    require(probe.world && probe.sync && controller && probe.reader, "allocate lifecycle resources");
    require(dvm_native_context_bind(controller), "bind controller");
    void *registers = dvm_native_context_registers(controller);
    DVM_WorldParticipant *participant = dvm_world_join(probe.world, controller);
    require(participant && dvm_test_cpp_epochs(probe.world, participant), "public C++98 epoch ABI");
    require(idle || dvm_world_enter(participant), "controller admission");
    require(dvm_world_read_begin(participant), "controller borrow before seal");
    require(!dvm_world_seal_last_owner(participant), "controller sealed with its own borrow");
    require(dvm_world_has_borrowers(probe.world) && dvm_world_read_end(participant),
            "failed seal preserves controller borrow");
    FVM_PortThread *thread = fvm_port_thread_start(service, &probe);
    require(thread != NULL, "start actual service thread");
    await_stage(&probe, 1);
    require(!dvm_world_seal_last_owner(participant), "nested service blocks sealing");
    advance(&probe, 2);
    await_stage(&probe, 3);
    require(!dvm_world_seal_last_owner(participant), "outer service borrow still blocks sealing");
    advance(&probe, 4);
    await_stage(&probe, 5);
    require(dvm_world_seal_last_owner(participant), "seal after all borrows finish");
    require(!dvm_world_read_begin(participant), "controller borrow reopened sealed world");
    require(!dvm_world_read_end(participant) && !dvm_world_has_borrowers(probe.world),
            "closed controller borrow remains empty");
    unsigned long epoch = dvm_world_retire_epoch(probe.world);
    require(epoch && (idle || !dvm_world_epoch_elapsed(probe.world, epoch)), "retirement observes active owner");
    advance(&probe, 6);
    fvm_port_thread_join(thread);
    require(idle || dvm_world_leave(participant), "final controller admission leave");
    require(dvm_world_epoch_elapsed(probe.world, epoch), "retirement completes after actual reader join");
    require(registers == dvm_native_context_registers(controller), "stable live register identity");
    require(dvm_world_detach(participant) && dvm_native_context_unbind(controller), "controller detach");
    require(dvm_native_context_destroy(probe.reader) && dvm_native_context_destroy(controller),
            "destroy joined contexts");
    require(dvm_world_destroy(probe.world), "destroy empty world");
    fvm_port_sync_destroy(probe.sync);
    puts("PASS sealed borrows: nested=2 joined=1 late=0 registers=stable");
    return 0;
}
