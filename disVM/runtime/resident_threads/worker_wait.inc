/* Copyright (c) 2026 bhgv
 * EN: Rootless waits, joins and bounded metadata accounting.
 * RU: Ожидания без корней, join и короткий учёт метаданных.
 */


/* EN: Copy the stop/failure predicate under service.sync; no arguments or heap access. Return
 * nonzero when execution must finish.
 * RU: Прочитать остановку/ошибку под service.sync без аргументов и доступа к куче. Ненулевой
 * результат требует завершения.
 */
static int stopping(void)
{
    int result;
    fvm_port_sync_lock(service.sync);
    result = service.stopping || service.failed;
    fvm_port_sync_unlock(service.sync);
    return result;
}


/* EN: Wake every registered resident for stop/failure. Caller holds service.sync, pinning each
 * borrowed condition; no heap access or result.
 * RU: Разбудить процессы для остановки/ошибки. Вызывающий удерживает service.sync и время жизни
 * условий; доступа к куче и результата нет.
 */
static void stop_dedicated_waits(void)
{
    ManagedWorker *worker;
    for (worker = service.wake_slots; worker; worker = worker->wake_next)
    {
        fvm_port_sync_lock(worker->wake);
        worker->wake_stopped = 1;
        fvm_port_sync_signal_all(worker->wake);
        fvm_port_sync_unlock(worker->wake);
    }
}


/* EN: Unlink borrowed worker from the wake registry before publishing done. Caller holds
 * service.sync; no freeing or result.
 * RU: Убрать заимствованный worker из реестра пробуждений до публикации done. Вызывающий
 * удерживает service.sync; освобождения и результата нет.
 */
static void unregister_dedicated_wait(ManagedWorker *worker)
{
    ManagedWorker **slot = &service.wake_slots;
    while (*slot && *slot != worker)
    {
        slot = &(*slot)->wake_next;
    }
    if (*slot)
    {
        *slot = worker->wake_next;
    }
}


/* EN: Latch failure with stable borrowed message, wake residents, then call cancel_waits outside
 * service.sync. Caller retains its heap phase and must retire safely.
 * RU: Зафиксировать ошибку с заимствованным message, разбудить процессы и вызвать cancel_waits вне
 * service.sync. Вызывающий сохраняет фазу кучи и обязан безопасно завершиться.
 */
static void fail(const char *message)
{
    fprintf(stderr, "disvm managed worker: %s\n", message);
    fvm_port_sync_lock(service.sync);
    service.failed = 1;
    stop_dedicated_waits();
    fvm_port_sync_signal_all(service.sync);
    fvm_port_sync_unlock(service.sync);
    service.cancel_waits();
}


/* EN: Release rootless owner admission and record site. Return zero without leaving if TLS still
 * owns a turn/process or release fails; caller owns no managed roots.
 * RU: Освободить допуск владельца без корней и записать site. Вернуть ноль без выхода при живом
 * ходе/процессе в TLS или отказе; у вызывающего нет управляемых корней.
 */
static int leave_empty(DVM_NativeWorldSite site)
{
    DVM_NativeContext *context = dvm_native_context_current();
    if (!context || !up || up->env != &up->defenv || up->prog || dvm_native_context_process(context) ||
        dvm_native_context_turn(context))
    {
        return 0;
    }
    /* NATIVEWORLD: release rootless admission before sleeping or joining. */
    if (!dvm_native_owner_leave(dvm_native_owner_current()))
    {
        return 0;
    }
    dvm_native_execution()->world_last_exit = site;
    return 1;
}


/* EN: Inspect borrowed worker readiness/mailboxes outside heap admission. Caller pins its own live
 * guest; return nonzero for queued work, without taking a message.
 * RU: Проверить готовность/почту заимствованного worker вне допуска к куче. Вызывающий удерживает
 * свой живой процесс; вернуть наличие работы, не извлекая сообщение.
 */
static int dedicated_work_pending(ManagedWorker *worker)
{
    DVM_RuntimeProcess *resident = (DVM_RuntimeProcess *)worker->guest;
    return __atomic_load_n(&resident->queued, __ATOMIC_ACQUIRE) || dvm_messages_pending(resident->messages, 0, 0) ||
           dvm_messages_pending(resident->messages, 1, 0);
}


/* EN: Wait on borrowed worker wake until stop or readiness. Only its resident calls this outside
 * admission; worker and guest remain live. No result.
 * RU: Ждать остановку либо готовность на wake заимствованного worker. Вызывает только его процесс
 * вне допуска; worker и guest живы. Результата нет.
 */
static void wait_dedicated_work(ManagedWorker *worker)
{
    fvm_port_sync_lock(worker->wake);
    while (!worker->wake_stopped && !dedicated_work_pending(worker))
    {
        publish_resident_phase(worker, DVM_RESIDENT_WAIT_WORK);
        increment_resident_counter(&worker->observed_waits);
        fvm_port_sync_wait(worker->wake);
    }
    fvm_port_sync_unlock(worker->wake);
    publish_resident_phase(worker, DVM_RESIDENT_SCHEDULER);
}


/* EN: Take at most one native reply into borrowed worker outside admission. Only its owner calls;
 * publish transit before dequeue and clear it if empty. No result.
 * RU: Извлечь один нативный ответ в заимствованный worker вне допуска. Вызывает только владелец;
 * опубликовать transit до извлечения, снять при пустой очереди. Результата нет.
 */
static void stage_dedicated_reply(ManagedWorker *worker)
{
    DVM_RuntimeProcess *resident = (DVM_RuntimeProcess *)worker->guest;
    __atomic_store_n(&worker->reply_in_transit, 1, __ATOMIC_RELEASE);
    worker->staged_reply = dvm_messages_take(resident->messages, 1, 0);
    if (!worker->staged_reply)
    {
        __atomic_store_n(&worker->reply_in_transit, 0, __ATOMIC_RELEASE);
    }
}


/* EN: Leave rootless admission, join finished slots, then reenter. idle selects waiting,
 * rendezvous selects controller pacing, optional pending avoids sleeping with work. Return zero on
 * refused release/stop; reentry or join failure is fatal. Reapers may overlap; only metadata uses
 * service.sync.
 * RU: Выйти из допуска без корней, присоединить завершённые слоты, вернуться. idle задаёт
 * ожидание, rendezvous — режим контроллера, pending запрещает сон при работе. Вернуть ноль при
 * отказе выхода/остановке; ошибка входа/join фатальна. Сборщики слотов параллельны; service.sync
 * защищает только метаданные.
 */
static int yield_empty(int idle, int rendezvous, int (*pending)(void))
{
    ManagedWorker **slot = &service.helpers;
    ManagedWorker *finished = NULL;
    ManagedWorker *worker;
    unsigned long generation;
    int wait_for_work;
    fvm_port_sync_lock(service.sync);
    generation = service.generation;
    fvm_port_sync_unlock(service.sync);
    if (pending && pending())
    {
        return !stopping();
    }
    fvm_port_sync_lock(service.sync);
    wait_for_work = idle && !rendezvous;
    while (*slot)
    {
        worker = *slot;
        if (worker->done && !worker->guest)
        {
            *slot = worker->next;
            worker->next = finished;
            finished = worker;
            service.reaping++;
        }
        else
        {
            slot = &worker->next;
        }
    }
    fvm_port_sync_unlock(service.sync);
    if (!leave_empty(DVM_NW_SITE_IDLE))
    {
        /* The caller still owns the heap. Keep every unjoined slot visible
         * to shutdown even when rootless admission is refused. */
        fvm_port_sync_lock(service.sync);
        while (finished)
        {
            worker = finished;
            finished = worker->next;
            worker->next = service.helpers;
            service.helpers = worker;
            service.reaping--;
        }
        fvm_port_sync_unlock(service.sync);
        return 0;
    }
    for (worker = finished; worker; worker = worker->next)
    {
        if (!dvm_thread_registrar_join(service.registrar, worker->thread))
        {
            fprintf(stderr, "disvm: cannot join registered worker\n");
            abort();
        }
    }
    if (current_guest && idle && !finished)
    {
        wait_dedicated_work(current_guest);
        if (stopping())
        {
            return 0;
        }
        stage_dedicated_reply(current_guest);
    }
    else if (wait_for_work && !finished)
    {
        fvm_port_sync_lock(service.sync);
        while (service.generation == generation && !service.stopping && !service.failed)
        {
            fvm_port_sync_wait(service.sync);
        }
        fvm_port_sync_unlock(service.sync);
    }
    else if (idle)
    {
        fvm_port_thread_sleep_us(1000);
    }
    /* NATIVEWORLD: restore owner admission after the rootless wait. */
    DVM_NativeExecution *execution = dvm_native_execution();
    execution->world_entry_site = DVM_NW_SITE_IDLE;
    execution->world_entry_pending = 1;
    if (!dvm_native_owner_enter(dvm_native_owner_current()))
    { /* Returning to a managed error handler requires ACTIVE ownership. */
        fprintf(stderr, "disvm: cannot reenter after empty scheduler yield\n");
        exit(1);
    }
    execution->world_entry_pending = 0;
    while (finished)
    {
        worker = finished;
        finished = worker->next;
        /* EN: Concurrent rootless reapers share only this metadata update.
 * Join, diagnostics and slot destruction remain outside service.sync.
 * RU: Параллельные сборщики слотов защищают только обновление учёта.
 * Join, диагностика и удаление слота выполняются вне service.sync.
 */
        fvm_port_sync_lock(service.sync);
        service.reaped++;
        service.reaping--;
        service.retired_turns += worker->turns;
        service.retired_releases += worker->owner.releases;
        service.retired_reentries += worker->owner.reentries;
        fvm_port_sync_unlock(service.sync);
        if (service.diagnostics)
        {
            fprintf(stderr,
                    "NATIVEWORKERS_REAP id=%lu joined=1 turns=%lu releases=%lu reentries=%lu\n",
                    worker->id,
                    worker->turns,
                    worker->owner.releases,
                    worker->owner.reentries);
        }
        if (worker->wake)
        {
            fvm_port_sync_destroy(worker->wake);
        }
        free(worker);
    }
    return !stopping();
}
