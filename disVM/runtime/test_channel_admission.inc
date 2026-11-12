/* Copyright (c) 2026 bhgv
 * EN: Native admission, suspended continuation and return fault probes.
 * RU: Нативный допуск, приостановленные продолжения и отказы возврата.
 * Private fixture fragment; preserves the original test composition order.
 */


/* Exercise the production no-release path while a different guest owns I/O. */
int __wrap_dvm_runtime_gc_native_try_leave(void)
{
    check(!verify_sleep_cancel_handoff, "cancelled sleep skips native release");
    if (io_refusals)
    {
        io_refusals--;
        puts("PASS native I/O admission refusal");
        return -1;
    }
    if (isolation_cancel_target)
    {
        pthread_mutex_lock(&gate);
        isolation_wait_registered = 1;
        pthread_cond_broadcast(&waiting);
        while (!dedicated_cancel_posted)
        {
            pthread_cond_wait(&waiting, &gate);
        }
        pthread_mutex_unlock(&gate);
    }
    return protected_output ? 0 : __real_dvm_runtime_gc_native_try_leave();
}


FVM_Result __real_dvm_runtime_read_stdin(void *bytes, size_t capacity, size_t *received);
FVM_Result
__real_dvm_runtime_write_stdio(int descriptor, const void *bytes, size_t capacity, size_t *written, int *released);


ssize_t __real_splice(
    int source, off64_t *source_offset, int destination, off64_t *destination_offset, size_t size, unsigned int flags);


/* Fail before inspecting data after a broken ownership transition. */
static void check(int condition, const char *reason)
{
    if (!condition)
    {
        fprintf(stderr, "channel guest wait: %s\n", reason);
        exit(10);
    }
}


static __thread int inject_native_return; /* The first mover requests a private fault. */
int __real_dvm_native_owner_progress(DVM_HeapOwner *owner, unsigned long epoch);


/* Inject only after reacquiring NativeWorld. The saved guest and queue remain
 * untouched for the runtime's refusal check. */
static void corrupt_native_return(void)
{
    static const size_t offsets[] = {
        offsetof(REG, FP),
        offsetof(REG, SP),
        offsetof(REG, EX),
        offsetof(REG, TS),
        offsetof(REG, PC),
        offsetof(REG, M),
        offsetof(REG, MP),
        offsetof(REG, s),
        offsetof(REG, m),
        offsetof(REG, d),
        offsetof(REG, xpc),
    };
    static const size_t word_offsets[] = {offsetof(REG, t), offsetof(REG, st), offsetof(REG, dt)};
    const size_t word_count = sizeof(word_offsets) / sizeof(word_offsets[0]);
    const size_t pointer_count = sizeof(offsets) / sizeof(offsets[0]);
    const char *selection = getenv("DVM_TEST_NATIVE_RETURN_FIELD");
    unsigned long field;
    void *replacement;
    unsigned char *slot;

    if (!selection)
    {
        return;
    }
    field = strtoul(selection, NULL, 10);
    check(field <= pointer_count + word_count, "unknown native return fixture field");
    if (field == pointer_count + word_count)
    {
        DVM_R.IC ^= 1;
        return;
    }
    if (field >= pointer_count)
    {
        WORD value;
        slot = (unsigned char *)&DVM_R + word_offsets[field - pointer_count];
        memmove(&value, slot, sizeof(value));
        value ^= 1;
        memmove(slot, &value, sizeof(value));
        return;
    }
    slot = (unsigned char *)&DVM_R + offsets[field];
    memmove(&replacement, slot, sizeof(replacement));
    replacement = replacement == NULL ? H : NULL;
    memmove(slot, &replacement, sizeof(replacement));
}


int __wrap_dvm_native_owner_progress(DVM_HeapOwner *owner, unsigned long epoch)
{
    int ok = __real_dvm_native_owner_progress(owner, epoch);

    if (ok && inject_native_return)
    {
        inject_native_return = 0;
        corrupt_native_return();
    }
    return ok;
}

static DVM_WorldRoots real_root_visitor;  /* Production callback retained through the fixture. */
static DVM_GCWorkers *root_probe_workers; /* Borrowed service for the explicit suspended-root run. */
static int root_probe_reported;           /* Sole probe result, observed after acknowledgment. */


static int native_root_probe; /* Immutable strict-backend selection before worker startup. */
DVM_GCWorkers *__real_dvm_gc_workers_create(const DVM_GCWorkersConfig *config);


/* Check that a stopped collector uses the saved native continuation roots.
 * Only the non-root decoded destination field is perturbed, and restored
 * before the collector leaves this callback. */
static void verify_suspended_roots(DVM_NativeContext *context, void *registers, size_t size, void *process, void *user)
{
    Prog *guest = process;
    REG *current = registers;

    if (guest && guest == first_process && guest->state == Prelease && size == sizeof(*current))
    {
        REG saved;
        void *original = current->d;


        memmove(&saved, &guest->R, sizeof(saved));
        current->d = saved.d ? NULL : H;
        real_root_visitor(context, registers, size, process, user);
        current->d = original;
        check(!memcmp(&guest->R, &saved, sizeof(saved)), "collector overwrote the saved suspended registers");
        if (!root_probe_reported)
        {
            root_probe_reported = 1;
            puts("PASS collector preserves suspended continuation roots");
        }
        return;
    }
    real_root_visitor(context, registers, size, process, user);
}


/* Install the probe only for the explicit collector control run. */
DVM_GCWorkers *__wrap_dvm_gc_workers_create(const DVM_GCWorkersConfig *config)
{
    DVM_GCWorkersConfig probe = *config;
    DVM_GCWorkers *workers;

    native_root_probe = getenv("DVM_TEST_SUSPENDED_ROOTS") && probe.concurrent_fast;
    if (getenv("DVM_TEST_SUSPENDED_ROOTS") && probe.roots)
    {
        real_root_visitor = probe.roots;
        probe.roots = verify_suspended_roots;
    }
    workers = __real_dvm_gc_workers_create(&probe);
    if (native_root_probe || probe.roots == verify_suspended_roots)
    {
        root_probe_workers = workers;
    }
    return workers;
}


/* [GC:] EN: Borrow the current suspended guest and live workers; execute one
 * real Fast attempt while perturbing only this thread's decoded destination.
 * Verify the saved continuation stays byte-identical, then restore live d.
 * No world stop, foreign execution or register reload is performed. A failed
 * pass or changed continuation is an explicit fixture failure.
 * RU: Заимствовать текущего приостановленного гостя и живые workers; выполнить
 * настоящую попытку Fast при изменённом лишь своём decoded destination.
 * Проверить неизменность сохранённого продолжения и вернуть живой d. Без
 * остановки мира, чужого исполнения и восстановления регистров. Отказ прохода
 * либо изменение продолжения явно завершают тест с ошибкой. */
static void verify_native_suspended_roots(Prog *guest, DVM_GCWorkers *workers)
{
    check(guest && guest == dvm_native_context_process(dvm_native_context_current()) && guest->state == Prelease,
          "probe owns actual suspended guest");
    REG saved;
    DVM_GCWorkersState before;
    DVM_GCWorkersState after;
    void *destination = DVM_R.d;
    memmove(&saved, &guest->R, sizeof(saved));
    void *perturbed = saved.d ? NULL : H;
    DVM_R.d = perturbed;
    dvm_runtime_publish_dispatch_roots(guest);
    check(DVM_R.d == perturbed, "publication never reloads live registers");
    int progressed = dvm_gc_workers_state(workers, &before) && dvm_gc_workers_step_fast(workers) &&
                     dvm_gc_workers_state(workers, &after) && !after.failed && after.Fast.passes > before.Fast.passes;
    DVM_R.d = destination;
    check(progressed, "real native Fast attempt completes during suspended continuation");
    check(!memcmp(&guest->R, &saved, sizeof(saved)), "native Fast overwrote suspended registers");
    root_probe_reported = 1;
    puts("PASS collector preserves suspended continuation roots");
}
