/* Copyright (c) 2026 bhgv
 * EN: Main driver fragment for the generated native owner regression.
 * RU: Главный драйвер проверки нативных владельцев, включаемый генератором.
 */

/* EN: Own the domain and contexts, test concurrent work and native waits.
 * argv selects locked polling. Return zero after exact arithmetic, stopped
 * roots, metadata isolation and complete joins. No managed pointers are owned.
 * RU: Создать домен и контексты, проверить параллельную работу и native.
 * argv выбирает опрос под мьютексом. Вернуть ноль после проверки арифметики,
 * остановленных корней, TLS и завершения всех потоков. */
int main(int argc, char **argv)
{
    Shared shared;
    Worker workers[2];
    FVM_PortThread *threads[2];
    DVM_NativeContext *collector;
    DVM_WorldAdmission admission;
    unsigned int index;
    DVM_HeapOwnerStats private_copy;
    unsigned long example_clock = ULONG_MAX-5;
    int example_status = 99;
    memset(&shared,0,sizeof(shared));
    memset(workers,0,sizeof(workers));
    shared.world = dvm_world_create_serial(argc > 1 && strcmp(argv[1],"locked") == 0);
    shared.control = fvm_port_sync_create();
    shared.operand = fvm_port_sync_create();
    collector = dvm_native_process_create(NULL);
    check(shared.world && shared.control && shared.operand && collector,"allocate domain");
    check(dvm_native_context_bind(collector),"bind collector");
    for (index = 0; index < 2; index++)
    {
        workers[index].shared = &shared;
        workers[index].identity = index;
        workers[index].context = dvm_native_process_create(NULL);
        check(workers[index].context != NULL,"allocate owner");
        threads[index] = fvm_port_thread_start(execute,&workers[index]);
        check(threads[index] != NULL,"start real OS owner");
    }
    /* Both owners remain active until main acknowledges their overlap. */
    wait_flag(&shared, &shared.overlap_verified);
    DVM_WorldState concurrent;
    check(dvm_world_state(shared.world, &concurrent) && concurrent.active == 2 &&
        dvm_world_exclusive_waiting(shared.world) == 0, "two simultaneous native owners");
    fvm_port_sync_lock(shared.control);
    shared.release_first = 1;
    fvm_port_sync_signal_all(shared.control);
    fvm_port_sync_unlock(shared.control);
    wait_flag(&shared,&shared.progressed);
    check(!dvm_native_owner_enter(workers[0].owner),"reject foreign owner");
    check(!dvm_native_owner_statistics(workers[0].owner,&private_copy),"reject foreign private counter read");
    check(!dvm_world_destroy(shared.world),"reject live world destruction");
    for (index = 0; index < SCANS; index++)
    {
        scan_roots(&shared,collector,1);
    }
    fvm_port_sync_lock(shared.control);
    shared.stress = 1;
    fvm_port_sync_signal_all(shared.control);
    fvm_port_sync_unlock(shared.control);
    for (index = 0; index < SCANS; index++)
    {
        scan_roots(&shared,collector,0);
    }
    fvm_port_sync_lock(shared.control);
    while (shared.done != 2)
    {
        fvm_port_sync_wait(shared.control);
    }
    fvm_port_sync_unlock(shared.control);
    scan_roots(&shared,collector,0);
    check(workers[0].snapshot.releases == 2 && workers[1].snapshot.releases >= 257 &&
        workers[0].snapshot.gc_progress >= SCANS,"independent overlapping native waits");
    check(shared.used == 3 && shared.order[0] == 0 && shared.order[1] == 1 &&
        shared.order[2] == 0,"explicit initial arithmetic handshake");
    check(shared.value == workers[0].iterations+workers[1].iterations,"no lost shared adds");
    check(dvm_world_admission(shared.world,&admission) && admission.serial &&
        !admission.waiting && !dvm_world_exclusive_waiting(shared.world) && !admission.contentions &&
        admission.admissions >= 2*STRESS_TURNS+259,"native admissions have no exclusive contenders");
    fvm_port_sync_lock(shared.control);
    shared.retire = 1;
    fvm_port_sync_signal_all(shared.control);
    fvm_port_sync_unlock(shared.control);
    for (index = 0; index < 2; index++)
    {
        fvm_port_thread_join(threads[index]);
        check(dvm_native_context_destroy(workers[index].context),"destroy joined owner");
    }
    check(dvm_native_owner_attach(dvm_native_owner_current(),shared.world),"attach last owner");
    check(dvm_native_owner_enter(dvm_native_owner_current()),"last owner enter");
    register_isolation();
    check(dvm_example_native_wait(dvm_native_owner_current(),example_epoch,&example_clock,
        example_failure,&example_clock,&example_status) && example_status == -7,
        "tutorial negative native status returned after reentry");
    check(dvm_native_owner_statistics(dvm_native_owner_current(),&private_copy) &&
        private_copy.gc_progress == 9 && private_copy.releases == 1 && private_copy.reentries == 1,
        "unsigned epoch wrap arithmetic");
    check(dvm_native_context_object_acquire(collector),"hold guard accounting");
    check(!dvm_native_owner_try_share(dvm_native_owner_current(), 3),
        "reject shared transition while a guard is held");
    check(dvm_native_context_object_release(collector),"release guard accounting");
    check(dvm_native_owner_try_share(dvm_native_owner_current(), 3),
        "convert the owner ACTIVE phase to SHARED");
    check(dvm_world_shared_peak(shared.world) == 1,"record shared occupancy");
    check(dvm_native_context_object_acquire(collector),"hold shared guard accounting");
    check(!dvm_native_owner_resume_shared(dvm_native_owner_current()),
        "reject ACTIVE return while a guard is held");
    check(dvm_native_context_object_release(collector),"release shared guard accounting");
    check(dvm_native_owner_resume_shared(dvm_native_owner_current()) &&
        dvm_native_owner_progress(dvm_native_owner_current(), 3),
        "promote uncontended shared turn and acknowledge epoch");
    check(dvm_native_owner_statistics(dvm_native_owner_current(),&private_copy) &&
        private_copy.phase == DVM_HEAP_ACTIVE && private_copy.releases == 2 &&
        private_copy.reentries == 2 && !private_copy.pending_progress,
        "shared handoff preserves the native lease accounting");
    shared.joined = 0;
    shared.retire = 0;
    FVM_PortThread *service = fvm_port_thread_start(idle_service, &shared);
    check(service != NULL, "start idle GC service");
    wait_flag(&shared, &shared.joined);
    check(dvm_native_owner_seal(dvm_native_owner_current()), "seal with idle GC service");
    fvm_port_sync_lock(shared.control);
    shared.retire = 1;
    fvm_port_sync_signal_all(shared.control);
    fvm_port_sync_unlock(shared.control);
    fvm_port_thread_join(service);
    {
        FVM_PortThread *late = fvm_port_thread_start(late_join,shared.world);
        check(late != NULL,"start late registration attempt");
        fvm_port_thread_join(late);
    }
    check(dvm_native_owner_leave(dvm_native_owner_current()),"last owner leave");
    check(dvm_native_owner_detach(dvm_native_owner_current()),"last owner detach");
    check(dvm_native_context_unbind(collector),"unbind collector");
    check(dvm_native_context_destroy(collector),"destroy collector");
    check(dvm_world_destroy(shared.world),"destroy empty domain");
    fvm_port_sync_destroy(shared.operand);
    fvm_port_sync_destroy(shared.control);
    puts("PASS per-owner native leases: independent overlapping calls/epochs, guarded arithmetic, 201 root scans, sealed registration, complete joins");
    return 0;
}
