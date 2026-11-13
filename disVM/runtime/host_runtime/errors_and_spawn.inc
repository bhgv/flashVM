/* Copyright (c) 2026 bhgv
 * EN: Owner-local errors, process initialization and native spawn; included once by main.c.
 * RU: Личные ошибки владельцев, создание процессов и native spawn; однократно подключается из main.c.
 */


/* Return the bound native owner's error buffer. Missing environment is a fatal
 * host protocol violation; no shared fallback may hide a foreign owner access.
 * Collector errors must never overwrite mutator text. No allocation/blocking. */
static char *owner_error(void)
{
    if (up && up->env && up->env->errstr)
    {
        return up->env->errstr;
    }
    fprintf(stderr, "disvm: error without a bound native environment\n");
    exit(1);
}


/* Preserve the current host error and unwind through the retained Proc error frames. */
void error(char *message)
{
    if (message != owner_error())
    {
        snprintf(owner_error(), ERRMAX, "%s", message);
    }
    nexterror();
}


/* Propagate the active error to the next enclosing native owner frame. */
void nexterror(void)
{
    if (!up || up->nerr <= 0)
    {
        fprintf(stderr, "disvm: %s\n", owner_error());
        exit(1);
    }
    if (!dvm_native_error_unwind())
    {
        fprintf(stderr, "disvm: native owner cleanup failed before error transfer\n");
        abort();
    }
    dvm_native_error_jump(up->estack[up->nerr], 1);
}


/* Run fallible construction with the real Proc jump-frame discipline. */
int kprotect(int (*callback)(void *), void *argument)
{
    int result;
    if (waserror())
    {
        return 0;
    }
    result = callback(argument);
    poperror();
    return result;
}


/* Re-raise after production cleanup. */
void krethrow(void)
{
    nexterror();
}


/* Save a native error for the retained module loader. */
void kwerrstr(char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vsnprint(owner_error(), ERRMAX, format, arguments);
    va_end(arguments);
}


/* EN: Compatibility no-op; synchronous host work retains the current owner.
 * No arguments/result, admission change, blocking or peer exclusion.
 * RU: Совместимый no-op: синхронная работа сохраняет владельца.
 * Без аргументов/результата, смены допуска, ожидания и исключения других гостей. */
void release(void)
{
}


/* EN: Compatibility no-op on return to the same native owner.
 * No arguments/result, register restore, blocking or peer exclusion.
 * RU: Совместимый no-op при возврате на того же владельца.
 * Без аргументов/результата, восстановления регистров, ожидания и эксклюзива. */
void acquire(void)
{
}


/* EN: Borrow this bound native owner's active Prog, or nil without an owner.
 * No shared queue lookup, blocking or ownership transfer. A returned Prog does
 * not grant access to another owner's heap fields; object guards still apply.
 * RU: Заимствовать Prog текущего владельца либо nil без владельца. Без поиска
 * в общей очереди, ожидания и передачи владения. Результат не разрешает доступ
 * к полям чужой кучи; объектные guards по-прежнему обязательны. */
Prog *currun(void)
{
    return up ? up->prog : nil;
}


/* EN: Publish borrowed process through the synchronized runqueue API.
 * Malformed/duplicate admission raises before modifying queue roots. This
 * publication does not acquire exclusive execution of the managed heap.
 * RU: Опубликовать заимствованный process через синхронизированную очередь.
 * Неверный/повторный допуск вызывает ошибку до изменения корней; публикация
 * не даёт эксклюзивного исполнения управляемой кучи. */
void addrun(Prog *process)
{
    if (!dvm_runqueue_append(process))
    {
        error("invalid runnable process admission");
    }
}


/* Withdraw the TLS guest for a wait or exit without changing its GC root links.
 * state is the retained scheduler reason. A foreign head raises before removal. */
Prog *delrun(int state)
{
    Prog *process = dvm_runqueue_pop_expected(currun(), state);
    if (!process)
    {
        error("invalid runnable process removal");
    }
    if (native_spawn && (state == Psend || state == Precv || state == Palt))
    {
        /* [GC:] EN: Caller still owns the channel registry mutex. Publish all
         * geometry before a peer can claim this waiter; dispatch must not
         * rewrite it until readiness is acquired after completion.
         * RU: Вызывающий ещё держит mutex каналов. Опубликовать геометрию до
         * захвата waiter партнёром; не переписывать её до acquire готовности. */
        process->R = DVM_R;
    }
    return process;
}


/* [GC:] EN: Publish this process's live register image at a dispatch boundary.
 * An acquire-ready sample excludes channel completion readers before writing;
 * a parked owner's image was already published by delrun under the queue mutex.
 * process remains owned by the current native thread; no state is restored.
 * RU: Опубликовать живой снимок process на границе назначения. Acquire готовности
 * исключает чтения завершения канала до записи; parked-снимок уже опубликован
 * delrun под mutex очереди. process принадлежит текущему потоку; без восстановления. */
void dvm_runtime_publish_dispatch_roots(Prog *process)
{
    if (!native_spawn || dvm_runqueue_is_ready(process))
    {
        process->R = DVM_R;
    }
}


/* EN: Dispose guest's endpoint once no producer can borrow the guest.
 * RU: Освободить endpoint гостя после ухода всех отправителей. */
static void drop_guest_messages(Prog *guest)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)guest;
    if (process->messages)
    {
        dvm_messages_stop(process->messages);
        while (dvm_messages_take(process->messages, 0, 0))
        { /* The request nodes are embedded in the guest. */
        }
        while (dvm_messages_take(process->messages, 1, 0))
        { /* Embedded replies are discarded on guest teardown. */
        }
        if (!dvm_messages_unregister(guest_messages, process->messages))
        {
            abort();
        }
        process->messages = NULL;
    }
}


/* EN: Initialize private process storage from an owner-local image.
 * RU: Инициализировать приватный Prog из локального образа владельца. */
static void initialize_process(Prog *process, Prog *parent, Modlink *module, const REG *initial)
{
    dvm_process_environment_init(process, parent);
    ((DVM_RuntimeProcess *)process)->parent_pid = parent ? parent->pid : 0;
    process->quanta = scheduler_quantum;
    process->xec = xec;
    process->exval = H;
    if (initial)
    {
        process->R = *initial;
    }
    else
    {
        process->R.M = module;
        process->R.MP = module->MP;
    }
}


/* EN: Reserve identity and initialize the private native message endpoint.
 * RU: Зарезервировать PID и подготовить приватный нативный ящик сообщений. */
int dvm_runtime_prepare_resident(Prog *process)
{
    DVM_RuntimeProcess *resident = (DVM_RuntimeProcess *)process;
    if (__atomic_load_n(&live, __ATOMIC_RELAXED) == PROCESS_MAX || nextpid == INT_MAX)
    {
        resident->startup_error = "process quota exceeded";
        return 0;
    }
    process->pid = ++nextpid;
    if (native_spawn)
    {
        resident->channel_close.kind = DVM_PROG_MESSAGE_CHANNEL_CLOSE;
        resident->channel_exit.kind = DVM_PROG_MESSAGE_CHANNEL_EXIT;
        resident->channel_cancel.kind = DVM_PROG_MESSAGE_CHANNEL_CANCEL;
        resident->cancel_wait.kind = DVM_PROG_MESSAGE_CANCEL_WAIT;
        resident->cancel_wait.sender = (uintptr_t)resident->parent_pid;
        resident->cancel_wait.request = (uint64_t)process->pid;
        resident->messages = dvm_messages_register(guest_messages, (uintptr_t)process->pid);
        if (!resident->messages)
        {
            resident->startup_error = exNomem;
            return 0;
        }
    }
    __atomic_fetch_add(&live, 1, __ATOMIC_RELAXED);
    return 1;
}


/* EN: Register a prepared resident and start its owner. Consumes prepared storage.
 * RU: Зарегистрировать готовый процесс и запустить его владельца. Поглощает prepared. */
static Prog *create_process(Prog *parent, Modlink *module, const REG *initial, Prog *prepared)
{
    Prog *process = prepared;
    if (!process)
    {
        process = calloc(1, sizeof(DVM_RuntimeProcess));
        if (!process)
        {
            error(exNomem);
        }
        initialize_process(process, parent, module, initial);
    }
    if (dvm_refcount_retain_u(&D2H(module)->ref) != DVM_REFCOUNT_LIVE)
    {
        free(process);
        error(exHeap);
    }
    DVM_RuntimeProcess *resident = (DVM_RuntimeProcess *)process;
    if (native_spawn && parent)
    {
        __atomic_store_n(&resident->startup_pending, 1, __ATOMIC_RELEASE);
        dvm_runqueue_set_state(process, Pready);
        /* Until registrar publication, the caller's frame still owns the arguments. */
        Heap *module_root = D2H(module);
        if (!dvm_runtime_root_add(module_root))
        {
            free(process);
            destroy(module);
            error(exHeap);
        }
        int started = dvm_managed_workers_start_guest(process);
        if (!dvm_runtime_root_remove(module_root))
        {
            abort();
        }
        if (!started)
        {
            const char *failure = resident->startup_error;
            if (resident->messages)
            {
                __atomic_fetch_sub(&live, 1, __ATOMIC_RELAXED);
            }
            drop_guest_messages(process);
            free(process);
            destroy(module);
            error((char *)(failure ? failure : "cannot create native guest thread"));
        }
    }
    else
    {
        if (!dvm_runtime_prepare_resident(process))
        {
            const char *failure = resident->startup_error;
            free(process);
            destroy(module);
            error((char *)failure);
        }
        dvm_runtime_process_root_add(process);
        addrun(process);
    }
    return process;
}


/* EN: Retained entry for callers without a prepared register image.
 * RU: Вход для вызывающих без подготовленного образа регистров. */
Prog *newprog(Prog *parent, Modlink *module)
{
    return create_process(parent, module, NULL, NULL);
}


/* EN: Native preparation ledger for a child whose source frame is still owned.
 * RU: Журнал подготовки потомка, исходный кадр ещё во владении создателя. */
typedef struct SpawnPreparation
{
    Prog *parent;    /* Current resident, whose source argument frame stays rooted. */
    Modlink *module; /* Rooted for the duration of private preparation. */
    Inst *entry;     /* Initial program counter captured by the spawn opcode. */
    Prog *initial;   /* Private stack and initial registers, freed by spawn on failure. */
    Prog *storage;   /* Private Prog allocation, transferred only after preparation. */
} SpawnPreparation;


/* EN: Allocate and fill only the child stack and its private process storage.
 * RU: Выделить и заполнить только приватный стек и Prog потомка. */
static int prepare_spawn_storage(void *argument)
{
    SpawnPreparation *request = argument;
    request->storage = calloc(1, sizeof(DVM_RuntimeProcess));
    if (!request->storage)
    {
        error(exNomem);
    }
    request->initial->R.M = request->module;
    request->initial->R.MP = request->module->MP;
    request->initial->R.PC = request->entry;
    newstack(request->initial);
    initialize_process(request->storage, request->parent, request->module, &request->initial->R);
    return 1;
}


/* EN: Prepare child storage in the caller's shared native interval, then publish.
 * parent/module/entry/initial remain borrowed; root module through preparation.
 * On failure restore admission and release the temporary root before raising.
 * Other native guests keep executing; no live register image is restored.
 * RU: Подготовить память потомка в shared-интервале, затем опубликовать.
 * parent/module/entry/initial заимствованы; module укоренён до конца подготовки.
 * При ошибке вернуть допуск и снять временный корень до исключения. Другие гости
 * продолжают исполняться; живые регистры из снимка не восстанавливаются. */
void dvm_runtime_spawn_prepared(Prog *parent, Modlink *module, Inst *entry, Prog *initial)
{
    SpawnPreparation request = {parent, module, entry, initial, NULL};
    if (native_spawn)
    {
        Heap *module_root = D2H(module);
        if (!dvm_runtime_root_add(module_root))
        {
            error(exHeap);
        }
        int shared = dvm_runtime_gc_shared_try_enter();
        if (shared <= 0)
        {
            if (!dvm_runtime_root_remove(module_root))
            {
                abort();
            }
            error("cannot prepare native guest outside exclusive admission");
        }
        int prepared = kprotect(prepare_spawn_storage, &request);
        dvm_runtime_gc_shared_finish();
        if (!dvm_runtime_root_remove(module_root))
        {
            abort();
        }
        if (!prepared)
        {
            free(request.storage);
            krethrow();
        }
    }
    else if (!kprotect(prepare_spawn_storage, &request))
    {
        free(request.storage);
        krethrow();
    }
    (void)create_process(parent, module, NULL, request.storage);
}
