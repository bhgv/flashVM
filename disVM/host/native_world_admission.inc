/* Copyright (c) 2026 bhgv
 * [GC:] EN: Native execution admission and reader epochs.
 * RU: Допуск нативного исполнения и эпохи читателей.
 */


/* [GC:] EN: A new active interval cannot overlap an already requested root scan.
 * RU: Заимствовать свой participant. shared выбирает ограниченный режим, promote — возврат из него,
 * try_only запрещает ожидание. Вернуть ноль при нарушении фазы/конфликте; другие гости не
 * исключаются. */
static int enter_world(DVM_WorldParticipant *participant, int shared, int promote, int try_only)
{
    DVM_NativeWorld *world;
    if (dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    if (!owns(participant))
    {
        return 0;
    }
    if (participant->service && (!shared || promote || !try_only))
    {
        return 0;
    }
    world = participant->world;
    fvm_port_sync_lock(world->sync);
    if (promote)
    {
        if (!participant->active || !participant->shared || participant->parked)
        {
            fvm_port_sync_unlock(world->sync);
            return 0;
        }
        if (!world->collector && !world->resuming)
        {
            participant->shared = 0;
            world->shared--;
            world->admissions++;
            fvm_port_sync_unlock(world->sync);
            return 1;
        }
        /* Publish before becoming inactive so the pending collection can proceed. */
        publish(participant);
        participant->active = 0;
        participant->shared = 0;
        world->active--;
        world->shared--;
        fvm_port_sync_signal_all(world->sync);
    }
    else if (participant->active || (shared && (!world->serial || world->sealed)))
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    /* EN: Experiment: wait for GC rendezvous only, never for other mutators.
     * RU: Эксперимент: ждать только барьер GC, без ожидания других исполнителей. */
    if (try_only && (world->collector || world->resuming))
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    while (world->collector || world->resuming)
    {
        fvm_port_sync_wait(world->sync);
    }
    world->admissions++;
    participant->admission_epoch = world->reclaim_epoch + 1;
    participant->active = 1;
    participant->shared = shared;
    world->active++;
    if (shared)
    {
        world->shared++;
        if (world->shared > world->shared_peak)
        {
            world->shared_peak = world->shared;
        }
        /* Wake observers of the updated shared occupancy. */
        fvm_port_sync_signal_all(world->sync);
    }
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* [GC:] EN: Announce a borrow before reading a published pointer. Nesting keeps the
 * oldest epoch until the last matching end. No heap admission is granted.
 * RU: Начать вложенный borrow своего participant до чтения указателя. Вернуть ноль при неверной
 * фазе/переполнении; ссылочное владение не меняется. */
int dvm_world_read_begin(DVM_WorldParticipant *participant)
{
    if (!owns(participant))
    {
        return 0;
    }
    DVM_NativeWorld *world = participant->world;
    fvm_port_sync_lock(world->sync);
    int ok = participant->read_depth != ULONG_MAX;
    if (ok)
    {
        if (!participant->read_depth)
        {
            participant->read_epoch = world->reclaim_epoch + 1;
        }
        participant->read_depth++;
    }
    fvm_port_sync_unlock(world->sync);
    return ok;
}


/* [GC:] EN: End one owner-local borrow after the final pointer access.
 * RU: Завершить borrow своего participant после последнего доступа. Ноль при неверной фазе/пустой
 * вложенности; известить ожидающих эпоху. */
int dvm_world_read_end(DVM_WorldParticipant *participant)
{
    if (!owns(participant))
    {
        return 0;
    }
    DVM_NativeWorld *world = participant->world;
    fvm_port_sync_lock(world->sync);
    int ok = participant->read_depth != 0;
    if (ok && --participant->read_depth == 0)
    {
        participant->read_epoch = 0;
    }
    fvm_port_sync_unlock(world->sync);
    return ok;
}


/* [GC:] EN: Exclusive callers use this before transferring a closed graph.
 * RU: Заимствовать живой world; под mutex проверить наличие явных borrow. Вернуть bool без
 * резервирования наблюдённого состояния. */
int dvm_world_has_borrowers(DVM_NativeWorld *world)
{
    int borrowed = 0;
    fvm_port_sync_lock(world->sync);
    for (DVM_WorldParticipant *reader = world->participants; reader && !borrowed; reader = reader->next)
    {
        borrowed = reader->read_depth != 0;
    }
    fvm_port_sync_unlock(world->sync);
    return borrowed;
}


/* [GC:] EN: Capture a retirement generation after withdrawing the pointer from new
 * readers. A zero result at counter exhaustion disables concurrent reclaim.
 * RU: Заимствовать живой world; под mutex выдать следующую эпоху. Ноль при неверном домене или
 * исчерпании счётчика; владельцы объектов не меняются. */
unsigned long dvm_world_retire_epoch(DVM_NativeWorld *world)
{
    if (!world)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    unsigned long epoch = 0;
    if (world->reclaim_epoch < ULONG_MAX - 1)
    {
        epoch = ++world->reclaim_epoch;
    }
    fvm_port_sync_unlock(world->sync);
    return epoch;
}


/* [GC:] EN: Observe grace-period completion without parking any participant. The
 * retained stopped collector is conservatively treated as a live reader.
 * RU: Заимствовать world; проверить завершение читателей эпохи epoch под mutex. Вернуть bool без
 * остановки участников и без резервирования результата. */
int dvm_world_epoch_elapsed(DVM_NativeWorld *world, unsigned long epoch)
{
    if (!world || !epoch)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    int elapsed = epoch <= world->reclaim_epoch && !world->collector;
    for (DVM_WorldParticipant *reader = world->participants; elapsed && reader; reader = reader->next)
    {
        elapsed =
            (!reader->active || reader->admission_epoch > epoch) && (!reader->read_depth || reader->read_epoch > epoch);
    }
    fvm_port_sync_unlock(world->sync);
    return elapsed;
}


/* [GC:] EN: Enter an ordinary mutator interval between GC stops.
 * RU: Ввести собственный participant в обычное исполнение; ждать лишь существующего эталонного GC,
 * не других гостей. Ноль при неверной фазе. */
int dvm_world_enter(DVM_WorldParticipant *participant)
{
    return enter_world(participant, 0, 0, 0);
}


/* [GC:] EN: Enter an audited shared interval between GC stops.
 * RU: Ввести свой participant в ограниченный shared-режим; защита полей остаётся у вызывающего.
 * Вернуть ноль при неверной фазе, ждать лишь эталонного GC. */
int dvm_world_enter_shared(DVM_WorldParticipant *participant)
{
    return enter_world(participant, 1, 0, 0);
}


/* [GC:] EN: Attempt one shared interval without waiting for GC.
 * RU: Однократно попробовать shared-вход своего participant. Ноль при конфликте без ожидания и без
 * изменения владения. */
int dvm_world_try_enter_shared(DVM_WorldParticipant *participant)
{
    return enter_world(participant, 1, 0, 1);
}


/* [GC:] EN: Keep the current turn only when it cannot jump a queued owner or GC.
 * RU: Перевести свой активный participant в shared, если ворота GC открыты. Ноль при неверной
 * фазе/конфликте; другие гости могут исполняться. */
int dvm_world_try_share(DVM_WorldParticipant *participant)
{
    if (dvm_native_context_object_count(dvm_native_context_current()) || !owns(participant))
    {
        return 0;
    }
    DVM_NativeWorld *world = participant->world;
    fvm_port_sync_lock(world->sync);
    if (!world->serial || world->sealed || world->collector || world->resuming || !participant->active ||
        participant->shared || participant->parked)
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    publish(participant);
    participant->shared = 1;
    world->shared++;
    if (world->shared > world->shared_peak)
    {
        world->shared_peak = world->shared;
    }
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* [GC:] EN: Promote borrowed owner participant from shared to ordinary
 * execution; return zero on invalid phase, never exclude peer mutators.
 * RU: Вернуть свой shared-participant в обычное исполнение с публикацией корней; не ждать других гостей. Ноль при неверной фазе. */
int dvm_world_promote_shared(DVM_WorldParticipant *participant)
{
    return enter_world(participant, 0, 1, 0);
}


/* [GC:] EN: Inactive means roots are stable even while the owner is blocked in I/O.
 * RU: Опубликовать корни своего participant и выйти из исполнения. Ноль при неверной фазе/живом
 * borrow; память участника сохраняется до detach. */
int dvm_world_leave(DVM_WorldParticipant *participant)
{
    DVM_NativeWorld *world;
    if (dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    if (!owns(participant))
    {
        return 0;
    }
    world = participant->world;
    fvm_port_sync_lock(world->sync);
    if (!participant->active || participant->parked)
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    publish(participant);
    participant->active = 0;
    if (participant->shared)
    {
        participant->shared = 0;
        world->shared--;
    }
    world->active--;
    fvm_port_sync_signal_all(world->sync);
    fvm_port_sync_unlock(world->sync);
    return 1;
}
