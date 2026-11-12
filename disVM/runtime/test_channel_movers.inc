/* Copyright (c) 2026 bhgv
 * EN: Channel construction, mover barriers and buffered ownership probes.
 * RU: Создание каналов, барьеры mover и владение буфером.
 * Private fixture fragment; preserves the original test composition order.
 */


/* Suspend the first transfer with its reservation and rooted channel intact. */
static void rendezvous_mover(void)
{
    DVM_NativeContext *owner = dvm_native_context_current();
    if (++moves == pause_at)
    {
        int released;
        uchar *saved_frame = DVM_R.FP;
        void *saved_destination = DVM_R.d;
        pthread_mutex_lock(&gate);
        first_owner = owner;
        first_process = dvm_native_context_process(owner);
        check(first_process != NULL, "first transfer is a real guest");
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
        if (dvm_runtime_shared_mode() == 2)
        {
            WORD private_word = 0;
            DVM_RangeAccess access = {0};


            check(dvm_managed_ranges_begin(&private_word, sizeof(private_word), NULL, 0, &access) == DVM_CELL_ACQUIRED,
                  "fixture acquires an operand guard");
            check(dvm_runtime_gc_native_try_leave() == -1 && dvm_runqueue_head() == first_process &&
                      dvm_native_context_process(owner) == first_process && !dvm_runqueue_owner(first_process),
                  "guarded suspension refuses without withdrawing the guest");
            check(dvm_range_access_end(&access), "fixture releases the operand guard");
        }
        released = dvm_runtime_gc_native_try_leave();
        check(released == 1, "first transfer releases NativeWorld");
        if (root_probe_workers)
        {
            if (native_root_probe)
            {
                verify_native_suspended_roots(first_process, root_probe_workers);
            }
            else
            {
                check(dvm_gc_workers_request(root_probe_workers, DVM_GC_CYCLES) &&
                          dvm_gc_workers_wait_idle(root_probe_workers) && root_probe_reported,
                      "collector visits saved roots before the mover can reenter");
            }
        }
        pthread_mutex_lock(&gate);
        while (!resume_mover)
        {
            pthread_cond_wait(&waiting, &gate);
        }
        pthread_mutex_unlock(&gate);
        {
            const char *phase = getenv("DVM_TEST_NATIVE_RETURN_PHASE");

            if (phase && strcmp(phase, "nested-leave") == 0)
            {
                dvm_runtime_gc_native_try_leave();
            }
            if (phase && strcmp(phase, "missing") == 0)
            {
                dvm_runtime_gc_native_enter(0);
            }
            if (phase && strcmp(phase, "negative") == 0)
            {
                dvm_runtime_gc_native_enter(-1);
            }
            if (phase && strcmp(phase, "noncanonical") == 0)
            {
                dvm_runtime_gc_native_enter(2);
            }
            inject_native_return = getenv("DVM_TEST_NATIVE_RETURN_FIELD") != NULL;
            dvm_runtime_gc_native_enter(released);
            if (phase && strcmp(phase, "duplicate") == 0)
            {
                dvm_runtime_gc_native_enter(released);
            }
        }
        check(dvm_native_context_current() == owner && dvm_native_context_process(owner) == first_process &&
                  dvm_runqueue_head() == first_process && DVM_R.FP == saved_frame && DVM_R.d == saved_destination,
              "native return preserves the original frame and result address");
        if (target_cancel && getenv("DVM_TEST_DEDICATED_BUFFER"))
        {
            if (isolate_waits)
            {
                pthread_mutex_lock(&gate);
                while (!isolation_wait_registered)
                {
                    pthread_cond_wait(&waiting, &gate);
                }
                pthread_mutex_unlock(&gate);
            }
            check(dvm_runtime_request_guest_cancel(waiting_pid), "post native channel cancellation");
            pthread_mutex_lock(&gate);
            dedicated_cancel_posted = 1;
            pthread_cond_broadcast(&waiting);
            pthread_mutex_unlock(&gate);
        }
        else if (target_cancel)
        {
            check(!dvm_runtime_cancel_guest_wait(first_process->pid) && dvm_runtime_cancel_guest_wait(waiting_pid) &&
                      dvm_runtime_cancel_guest_wait(waiting_pid),
                  "cancel exactly the active guest wait");
        }
        else if (cancel_wait)
        {
            dvm_runtime_cancel_native_waits();
        }
        if (fail_mover)
        {
            error("fixture channel mover failure");
        }
    }
    movw();
}


/* Instrument only the guest's buffered word channel. */
Channel *__wrap_dvm_runtime_channel_new(void *destination, Type *type, WORD capacity, void (*mover)(void), WORD width)
{
    if (type == &Tword && (capacity == 35 || capacity == 36))
    {
        host_cancel_mode = (int)capacity;
        parked_cancel_mode = 1;
    }
    if (type == &Tword && (capacity == 44 || capacity == 46))
    {
        host_cancel_on_publish = 1;
        host_cancel_no_idle_yield = capacity == 46;
        parked_cancel_mode = 1;
    }
    if (type == &Tword && (capacity == 40 || capacity == 43))
    {
        child_exit_cancel_mode = 1;
        child_exit_host_cancel = capacity == 43;
        parked_cancel_mode = 1;
    }
    if (type == &Tword && (capacity == 41 || capacity == 42))
    {
        int released = dvm_runtime_gc_native_try_leave();
        check(released == 1, "exit observer releases NativeWorld");
        pthread_mutex_lock(&gate);
        while (child_exit_wait_count < capacity - 40)
        {
            pthread_cond_wait(&waiting, &gate);
        }
        if (capacity == 42)
        {
            resume_mover = 1;
            pthread_cond_broadcast(&waiting);
        }
        pthread_mutex_unlock(&gate);
        dvm_runtime_gc_native_enter(released);
    }
    if (type == &Tword && capacity == 39)
    {
        parked_cancel_mode = 1;
    }
    if (type == &Tword && (capacity == 32 || capacity == 34))
    {
        parked_cancel_mode = capacity == 34 ? 2 : 1;
    }
    if (type == &Tword && (capacity == 33 || capacity == 37))
    {
        int released = dvm_runtime_gc_native_try_leave();
        check(released == 1, "cancellation coordinator releases NativeWorld");
        pthread_mutex_lock(&gate);
        while (parked_count < parked_cancel_mode)
        {
            pthread_cond_wait(&waiting, &gate);
        }
        pthread_mutex_unlock(&gate);
        if (capacity == 37)
        {
            dvm_runtime_gc_native_enter(released);
            return __real_dvm_runtime_channel_new(destination, type, capacity, mover, width);
        }
        if (getenv("DVM_TEST_DEDICATED_BUFFER"))
        {
            check(dvm_runtime_request_guest_cancel(parked_pid), "post cancellation to the parked owner");
            if (getenv("DVM_TEST_CANCEL_BOTH"))
            {
                check(second_parked_pid && dvm_runtime_request_guest_cancel(second_parked_pid),
                      "post cancellation to the second parked owner");
            }
            dvm_runtime_gc_native_enter(released);
        }
        else
        {
            check(!dvm_runtime_cancel_guest_wait(parked_pid), "native-only caller cannot touch a parked guest");
            dvm_runtime_gc_native_enter(released);
            check(!dvm_runtime_cancel_guest_wait(0) && dvm_runtime_cancel_guest_wait(parked_pid) &&
                      !dvm_runtime_cancel_guest_wait(parked_pid),
                  "parked waiter cancels exactly once through the scheduler");
        }
        puts("PASS cancelled scheduler channel wait");
    }
    if (type == &Tword && capacity == 31)
    {
        directory_fault_mode = 1;
    }
    if (type == &Tword && (capacity == 28 || capacity == 29))
    {
        broken_pipe_mode = (int)capacity;
    }
    if (type == &Tword && (capacity == 26 || capacity == 27 || capacity == 30))
    {
        stdout_alias_mode = (int)capacity;
    }
    if (type == &Tword && (capacity >= 23 && capacity <= 25))
    {
        stdout_cancel_mode = (int)capacity;
    }
    if (type == &Tword && (capacity >= 20 && capacity <= 22))
    {
        stdin_cancel_mode = (int)capacity;
    }
    if (type == &Tword && capacity == 18)
    {
        file_cancel_mode = 1;
    }
    if (type == &Tword && (capacity == 16 || capacity == 17 || capacity == 38 || capacity == 45))
    {
        sleep_mode = (int)capacity;
    }
    if (type == &Tword && capacity == 15)
    {
        io_refusals = 10;
    }
    if (type == &Tword && (capacity == 6 || capacity == 9 || capacity == 19))
    {
        int released = dvm_runtime_gc_native_try_leave();
        check(released == 1, "spare sender gate releases NativeWorld");
        pthread_mutex_lock(&gate);
        while (capacity == 6 ? !alt_wait_entered : (capacity == 19 ? !first_file_entered : !first_owner))
        {
            pthread_cond_wait(&waiting, &gate);
        }
        pthread_mutex_unlock(&gate);
        dvm_runtime_gc_native_enter(released);
        if (capacity == 6 && stdout_alias_mode == 27)
        {
            check(dvm_runtime_cancel_guest_wait(stdout_wait_pid), "queued alias write is cancellable");
        }
        if (capacity == 6 && (stdout_cancel_mode == 23 || stdout_cancel_mode == 24))
        {
            check(dvm_runtime_cancel_guest_wait(stdout_wait_pid), "pipe output is still cancellable");
        }
        if (capacity == 6 && (stdin_cancel_mode == 20 || stdin_cancel_mode == 22))
        {
            check(dvm_runtime_cancel_guest_wait(stdin_wait_pid), "pipe poll is still cancellable");
        }
        if (capacity == 6 && file_cancel_mode)
        {
            check(dvm_runtime_cancel_guest_wait(file_wait_pid), "queued file operation can be cancelled");
        }
        if (capacity == 6 && sleep_mode == 16)
        {
            check(dvm_runtime_cancel_guest_wait(sleeping_pid), "targeted sleep is still registered");
        }
    }
    if (type == &Tword && ((capacity >= 2 && capacity <= 8 && capacity != 6) || (capacity >= 10 && capacity <= 14)))
    {
        pause_at = capacity == 3 ? 3 : 1;
        fail_mover = capacity == 4;
        alt_bypass = capacity == 5;
        alternate_peer = capacity == 7;
        pool_refusal = capacity == 13;
        thread_retry = capacity == 14;
        worker_failure = capacity == 12;
        isolate_waits = capacity == 11;
        target_cancel = capacity == 10 || isolate_waits;
        cancel_wait = capacity == 8 || target_cancel;
        mover = rendezvous_mover;
    }
    return __real_dvm_runtime_channel_new(destination, type, capacity, mover, width);
}


/* Exercise finalizer admission while the foreign reservation is active. */
static int try_buffer_finalization(void *argument)
{


    extern void freechan(Heap *, int);
    freechan(D2H(argument), 0);
    return 1;
}


/* Start the real ALT only after a different guest owns the reservation.
 * block is the VM opcode's blocking flag. This test-only gate releases
 * NativeWorld while it waits and acquires it before entering ALT. */
void __wrap_xecalt(int block)
{
    int released;

    if (parked_cancel_mode || sleep_mode == 38 || getenv("DVM_TEST_CLOSE_PARKED") || getenv("DVM_TEST_ALT_STALE"))
    {
        __real_xecalt(block);
        return;
    }
    pthread_mutex_lock(&gate);
    alt_owner = dvm_native_context_current();
    if (!first_owner)
    {
        pthread_mutex_unlock(&gate);
        released = dvm_runtime_gc_native_try_leave();
        check(released == 1, "ALT gate releases NativeWorld");
        pthread_mutex_lock(&gate);
        while (!first_owner)
        {
            pthread_cond_wait(&waiting, &gate);
        }
        pthread_mutex_unlock(&gate);
        dvm_runtime_gc_native_enter(released);
    }
    else
    {
        pthread_mutex_unlock(&gate);
    }
    __real_xecalt(block);
    pthread_mutex_lock(&gate);
    if (alt_bypass)
    {
        resume_mover = 1;
        pthread_cond_signal(&waiting);
        puts("PASS ALT bypassed the reservation");
    }
    else if (alternate_peer)
    {
        check(alt_wait_entered && !resume_mover, "spare peer wins before transfer completes");
        resume_mover = 1;
        pthread_cond_broadcast(&waiting);
    }
    else
    {
        check(resume_mover, "ALT returned without waiting for the active transfer");
    }
    pthread_mutex_unlock(&gate);
}
