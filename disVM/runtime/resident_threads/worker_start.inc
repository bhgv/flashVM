/* Copyright (c) 2026 bhgv
 * EN: Registrar-mediated native resident startup.
 * RU: Запуск нативных процессов через регистратор.
 */


/* EN: Link borrowed worker and account startup under service.sync held across registrar
 * publication. Service owns slot until reaping/join; no result or allocation.
 * RU: Добавить worker и учесть запуск под service.sync, удерживаемым до публикации регистратора.
 * Сервис владеет слотом до сборки/join; результата и выделения памяти нет.
 */
static void install_worker(ManagedWorker *worker)
{
    service.created++;
    worker->next = service.helpers;
    service.helpers = worker;
    service.live++;
    if (service.live + 1 > service.peak)
    {
        service.peak = service.live + 1;
    }
}


/* EN: Registrar preflight for borrowed argument slot while requester holds service.sync. Return
 * zero on exhausted IDs or resident preparation failure; allocate a permanent ID only after
 * preparation.
 * RU: Проверка заимствованного слота argument регистратором при удерживаемом service.sync. Вернуть
 * ноль при исчерпании ID или ошибке подготовки процесса; назначить постоянный ID после подготовки.
 */
static int prepare_resident_slot(void *argument)
{
    ManagedWorker *worker = argument;
    if (service.next_id == ULONG_MAX || !dvm_runtime_prepare_resident(worker->guest))
    {
        return 0;
    }
    worker->id = ++service.next_id;
    worker->guest_pid = worker->guest->pid;
    return 1;
}


/* EN: Registrar commits borrowed argument slot with owned thread handle under requester
 * service.sync. Publish dedicated pointer, registry and process root before reply; no result.
 * RU: Регистратор фиксирует слот argument с дескриптором thread под service.sync вызывающего.
 * Опубликовать dedicated, реестр и корень процесса до ответа; результата нет.
 */
static void publish_resident_slot(void *argument, DVM_ThreadSlot *thread)
{
    ManagedWorker *worker = argument;
    worker->thread = thread;
    ((DVM_RuntimeProcess *)worker->guest)->dedicated = worker;
    install_worker(worker);
    dvm_runtime_process_root_add(worker->guest);
}


/* EN: Create a stable slot for borrowed rooted guest via registrar, waiting outside admission.
 * Return zero on invalid guest, stop or native allocation/start refusal; transfer slot to service
 * only on commit. Never execute another Prog.
 * RU: Создать стабильный слот для заимствованного укоренённого guest через регистратор, ожидая вне
 * допуска. Вернуть ноль при неверном guest, остановке, отказе выделения/старта; передать слот
 * сервису только при фиксации. Другой Prog не исполняется.
 */
static int start_helper(Prog *guest)
{
    ManagedWorker *worker;
    if (!guest || stopping())
    {
        return 0;
    }
    worker = calloc(1, sizeof(*worker));
    if (!worker)
    {
        return 0;
    }
    worker->guest = guest;
    worker->wake = fvm_port_sync_create();
    if (!worker->wake)
    {
        free(worker);
        return 0;
    }
    fvm_port_sync_lock(service.sync);
    worker->wake_next = service.wake_slots;
    service.wake_slots = worker;
    fvm_port_sync_unlock(service.sync);

    int released = dvm_runtime_gc_native_try_leave();
    if (released > 0)
    {
        /* Order creation before shutdown's first join request. The new
         * resident waits on its private condition until commit. */
        fvm_port_sync_lock(service.sync);
        if (!service.stopping && !service.failed)
        {
            (void)dvm_thread_registrar_start_prepared(
                service.registrar, execute, worker, prepare_resident_slot, publish_resident_slot);
        }
        fvm_port_sync_unlock(service.sync);
        dvm_runtime_gc_native_enter(released);
    }
    if (!worker->thread)
    {
        fvm_port_sync_lock(service.sync);
        unregister_dedicated_wait(worker);
        fvm_port_sync_unlock(service.sync);
        fvm_port_sync_destroy(worker->wake);
        free(worker);
        return 0;
    }
    fvm_port_sync_lock(worker->wake);
    worker->startup_committed = 1;
    fvm_port_sync_signal_all(worker->wake);
    fvm_port_sync_unlock(worker->wake);
    return 1;
}
