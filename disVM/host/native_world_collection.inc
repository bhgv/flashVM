/* Copyright (c) 2026 bhgv
 * [GC:] EN: Collector admission policy and coherent world observations.
 * RU: Политика допуска сборщика и согласованные наблюдения мира.
 */


/* [GC:] EN: Serialize collectors while allowing all mutators to run outside this mutex.
 * RU: world и collector заимствованы; quiescent запрещает парковку активных участников. Строгий
 * домен завершает процесс кодом 70. Эталон резервирует остановку либо возвращает ноль при конфликте. */
static int stop_begin(DVM_NativeWorld *world, DVM_NativeContext *collector, int quiescent)
{
    if (world && world->forbid_stops)
    {
        fprintf(stderr,
                "NATIVE_CONCURRENCY_ERROR component=world reason=exclusive-stop-forbidden quiescent=%d\n",
                quiescent);
        fflush(stderr);
        _Exit(70);
    }
    DVM_WorldParticipant *participant;
    if (dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    if (!world || !collector || dvm_native_context_current() != collector)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    if (world->collector || world->parked || world->resuming || (quiescent && world->active))
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    for (participant = world->participants; participant; participant = participant->next)
    {
        if (participant->context == collector || (quiescent && participant->read_depth))
        {
            fvm_port_sync_unlock(world->sync);
            return 0;
        }
    }
    if (!dvm_native_context_set_poll(collector, poll_collector, world))
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    world->collector = collector;
    if (!quiescent)
    {
        dvm_world_request_store(&world->request, 1);
        fvm_port_sync_signal_all(world->sync);
        while (world->active != world->parked)
        {
            fvm_port_sync_wait(world->sync);
        }
    }
    world->stopped = 1;
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* [GC:] EN: Request the retained stop rendezvous.
 * RU: Заимствовать world и привязанный collector; начать эталонную остановку с ожиданием владельцев.
 * В строгом домене код 70 до изменения; иначе ноль при конфликте. */
int dvm_world_stop_begin(DVM_NativeWorld *world, DVM_NativeContext *collector)
{
    return stop_begin(world, collector, 0);
}


/* [GC:] EN: Claim an already quiescent heap without asking active owners to park.
 * RU: Заимствовать world и collector; попробовать эталонный захват уже спокойной кучи. В строгом
 * домене код 70; иначе ноль без парковки при занятых участниках. */
int dvm_world_try_quiescent(DVM_NativeWorld *world, DVM_NativeContext *collector)
{
    return stop_begin(world, collector, 1);
}


/* [GC:] EN: Do not reserve admission. The caller must retry the full quiescence check.
 * RU: Заимствовать world; ограниченно ждать изменения состояния перед новым эталонным захватом.
 * Никакой допуск или владелец объекта не выдаётся. */
void dvm_world_wait_quiescent_retry(DVM_NativeWorld *world)
{
    fvm_port_sync_lock(world->sync);
    int busy = world->active || world->collector || world->parked || world->resuming;
    for (DVM_WorldParticipant *participant = world->participants; !busy && participant; participant = participant->next)
    {
        busy = participant->read_depth != 0;
    }
    if (busy)
    {
        fvm_port_sync_wait_ms(world->sync, 1);
    }
    fvm_port_sync_unlock(world->sync);
}


/* [GC:] EN: No active participant can mutate its published roots during this visit.
 * RU: Заимствовать world, visitor и user; вызвать visitor для стабильных корней только при своей
 * остановке. Ноль при неверном владении; мьютекс защищает обход участников. */
int dvm_world_visit(DVM_NativeWorld *world, DVM_WorldRoots visitor, void *user)
{
    DVM_WorldParticipant *participant;
    if (!world || !visitor)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    if (!world->stopped || !world->collector || world->collector != dvm_native_context_current())
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    for (participant = world->participants; participant; participant = participant->next)
    {
        visitor(participant->context, participant->registers, participant->register_size, participant->process, user);
    }
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* [GC:] EN: Wait for the old parked generation to drain before allowing a new stop request.
 * RU: Заимствовать world; снять только собственную эталонную остановку и дождаться выхода
 * припаркованных участников. Вернуть ноль при чужом/пустом захвате. */
int dvm_world_stop_end(DVM_NativeWorld *world)
{
    if (dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    if (!world)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    if (!world->stopped || !world->collector || world->collector != dvm_native_context_current())
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    world->stopped = 0;
    world->resuming = 1;
    world->collector = 0;
    dvm_world_request_store(&world->request, 0);
    fvm_port_sync_signal_all(world->sync);
    while (world->parked)
    {
        fvm_port_sync_wait(world->sync);
    }
    if (!dvm_native_context_clear_poll(dvm_native_context_current(), poll_collector, world))
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    world->resuming = 0;
    fvm_port_sync_signal_all(world->sync);
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* [GC:] EN: Observe one coherent state without joining a collection rendezvous.
 * RU: Скопировать под mutex состояние живого world в личный out. Ноль при NULL; снимок ничего не
 * резервирует и не передаёт владения. */
int dvm_world_state(DVM_NativeWorld *world, DVM_WorldState *out)
{
    if (!world || !out)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    out->participants = world->count;
    out->active = world->active;
    out->parked = world->parked;
    out->stopping = world->collector != 0;
    out->stopped = world->stopped;
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* [GC:] EN: Copy owner-private poll counters. participant must belong to the calling
 * bound context; out parameters are caller-owned. No cross-owner counter reads,
 * heap access, blocking or collection admission occurs. Returns zero on misuse.
 * RU: Из своего participant скопировать счётчики в личные fast/locked. Ноль при чужом владельце или
 * NULL; ожидания сборки нет. */
int dvm_world_owner_polls(DVM_WorldParticipant *participant, size_t *fast, size_t *locked)
{
    if (!owns(participant) || !fast || !locked)
    {
        return 0;
    }
    *fast = participant->fast_polls;
    *locked = participant->locked_polls;
    return 1;
}


/* [GC:] EN: Copy serialized admission diagnostics under the world mutex. world/out are
 * borrowed live native objects; no reservation, heap access or state mutation.
 * Returns zero for invalid arguments, one for a coherent snapshot in either mode.
 * RU: Заимствовать world и личный out; скопировать hosted-счётчики под mutex. Ноль при NULL; снимок
 * не является допуском. */
int dvm_world_admission(DVM_NativeWorld *world, DVM_WorldAdmission *out)
{
    if (!world || !out)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    out->serial = world->serial;
    out->waiting = 0;
    out->admissions = world->admissions;
    out->contentions = 0;
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* [GC:] EN: Check for active guest work in the borrowed live world. The mutex
 * protects participant membership and phase while the observation is made.
 * RU: Проверить живой world под mutex, исключая shared-службы. Вернуть наличие гостя без
 * резервирования его фазы. */
int dvm_world_has_active_guest(DVM_NativeWorld *world)
{
    if (!world)
    {
        return 0;
    }
    int found = 0;
    fvm_port_sync_lock(world->sync);
    for (DVM_WorldParticipant *participant = world->participants; participant; participant = participant->next)
    {
        if (participant->active && !participant->parked && !participant->service)
        {
            found = 1;
            break;
        }
    }
    fvm_port_sync_unlock(world->sync);
    return found;
}


/* EN: Observe waiting exclusive owners in world without its admission mutex.
 * RU: Узнать число ожидающих эксклюзивного допуска в world без его мьютекса. */
size_t dvm_world_exclusive_waiting(DVM_NativeWorld *world)
{
    (void)world;
    return 0;
}


/* [GC:] EN: Observe the highest simultaneous SHARED occupancy under the world mutex.
 * RU: Из живого world вернуть максимум одновременных shared-участников под mutex либо ноль для NULL;
 * владение не меняется. */
size_t dvm_world_shared_peak(DVM_NativeWorld *world)
{
    size_t peak;
    if (!world)
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    peak = world->shared_peak;
    fvm_port_sync_unlock(world->sync);
    return peak;
}


/* [GC:] EN: Close registrations and new borrows only for the last mutator.
 * participant is borrowed/current; any explicit borrow, including its own,
 * peers or invalid phase leave the domain open. Return zero on refusal. The
 * existing owner can finish and detach after success; no managed owner moves.
 * RU: Закрыть регистрацию и новые borrow только для последнего исполнителя.
 * participant заимствован/текущий; любой borrow, включая собственный, соседи
 * или неверная фаза сохраняют домен открытым. Ноль при отказе. После успеха
 * владелец может завершить работу и detach; управляемое владение не меняется. */
int dvm_world_seal_last_owner(DVM_WorldParticipant *participant)
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
    if (!world->serial || world->active != (size_t)participant->active || participant->service || participant->parked ||
        participant->shared || participant->read_depth ||
        (!participant->active && (world->collector || world->resuming)))
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    for (DVM_WorldParticipant *other = world->participants; other; other = other->next)
    {
        if (other != participant && (!other->service || other->read_depth))
        {
            fvm_port_sync_unlock(world->sync);
            return 0;
        }
    }
    world->sealed = 1;
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* [GC:] EN: Verify exact collecting owner under the world mutex, without traversing
 * roots. world borrowed/live; false for unbound/guarded/foreign or unstopped use.
 * A true result stays valid until this same collector ends its stop interval.
 * RU: Заимствовать world; под mutex проверить владение остановкой текущего контекста. Вернуть bool,
 * не выдавая нового допуска. */
int dvm_world_collector_owns(DVM_NativeWorld *world)
{
    DVM_NativeContext *context = dvm_native_context_current();
    int result;
    if (!world || !context || dvm_native_context_object_count(context))
    {
        return 0;
    }
    fvm_port_sync_lock(world->sync);
    result = world->stopped && world->collector == context;
    fvm_port_sync_unlock(world->sync);
    return result;
}
