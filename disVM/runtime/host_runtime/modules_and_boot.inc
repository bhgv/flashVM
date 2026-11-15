/* Copyright (c) 2026 bhgv
 * EN: Builtin services, module loading and initial process startup; included once by main.c.
 * RU: Встроенные службы, загрузка модулей и запуск первого процесса; однократно подключается из main.c.
 */


/* Initialize the actual descriptor-owner finalizer used by the supported input path. */
void sysinit(void)
{
    if (TFD)
    {
        return;
    }
    TFD = dtype(free_fd, sizeof(Sys_FD), nil, 0);
    if (!TFD)
    {
        error(exNomem);
    }
}


/* Accept only ordinary Dis; dynamic native libraries and signatures are unavailable. */
int dynldable(int fd)
{
    USED(fd);
    return 0;
}


/* Do not require signatures for local experimental module files. */
int mustbesigned(char *path, uchar *bytes, ulong length, Dir *metadata)
{
    USED(path);
    USED(bytes);
    USED(length);
    USED(metadata);
    return 0;
}


/* Signed payload verification is explicitly unsupported. */
int verifysigner(uchar *signature, int size, uchar *bytes, ulong length)
{
    USED(bytes);
    USED(length);
    USED(signature);
    USED(size);
    return 0;
}


/* EN: Reserve one slot under the registry mutex, then open outside that mutex.
 * path/mode are borrowed; asynchronous selects audited Sys calls. Return a
 * published descriptor or -1. A gate reserves even an unpublished/failed open.
 * RU: Занять слот под мьютексом реестра, затем открыть вне мьютекса.
 * path/mode заимствованы; asynchronous выбирает Sys. Вернуть дескриптор либо -1.
 * До публикации и при откате слот остаётся занят своим шлюзом. */
static int open_selected(const char *path, const char *mode, int asynchronous, DVM_ResourcePlan *selected)
{
    int index;
    DVM_FileGate *gate = dvm_file_gate_create();
    DVM_CloseJob *job = gate ? dvm_close_prepare() : NULL;
    FVM_PortFile *file = NULL;
    FVM_PortDirectory *directory = NULL;
    DVM_VfsHandle provider_output = {0}; /* Owned until slot publication; factory pins code/state. */
    FVM_Result status;
    if (!gate || !job)
    {
        dvm_file_gate_destroy(gate);
        dvm_close_discard(job);
        kwerrstr("cannot reserve native handle lifetime");
        return -1;
    }
    index = dvm_file_slots_reserve(gate, job);
    if (index < 0)
    {
        dvm_file_gate_destroy(gate);
        dvm_close_discard(job);
        kwerrstr("file handle quota exceeded");
        return -1;
    }
    status = dvm_open_wait_resource(path, mode, asynchronous, selected, &file, &directory, &provider_output);
    if (status == FVM_OK)
    {
        if (provider_output.ops)
        {
            dvm_file_slots_publish_resource(index, &provider_output);
        }
        else
        {
            dvm_file_slots_publish(index, file, directory);
        }
        return index;
    }
    DVM_FileSlot abandoned;
    if (!dvm_file_slots_take(index, 0, &abandoned))
    {
        fprintf(stderr, "disvm: native open reservation disappeared\n");
        exit(1);
    }
    dvm_file_gate_destroy(abandoned.gate);
    dvm_close_discard(abandoned.job);
    kwerrstr("cannot open native file or directory (status %d)", status);
    return -1;
}


/* EN/RU: Ordinary Sys opens capture their own selection; path/mode are borrowed.
 * Обычный Sys open захватывает свой selection; path/mode заимствованы. FD/-1. */
static int open_file_mode(const char *path, const char *mode, int asynchronous)
{
    return open_selected(path, mode, asynchronous, NULL);
}


/* EN/RU: Preserve selected directory identity through module I/O; return FD/-1.
 * Сохранить выбранную identity каталога при module I/O; вернуть FD/-1. */
int dvm_namespace_module_open(DVM_ResourcePlan *input)
{
    return input ? open_selected(input->names.native, "r", 0, input) : -1;
}


/* Loader-only native input: borrowed path/mode, synchronous protected lifetime.
 * Return the owned descriptor slot or -1; the caller closes it with kclose. */
static int open_file(const char *path, const char *mode)
{
    return open_file_mode(path, mode, 0);
}


/* EN: Open an already frozen absolute native module path. Namespace lookup and
 * cache selection happened together before native release; never resolve twice.
 * RU: Открыть frozen absolute native module path. Namespace/cache выбраны вместе
 * до native release; повторного разрешения пути нет. Return FD or -1 / FD либо -1. */
int kopen(char *path, int mode)
{
    if (!path || path[0] != '/' || mode != OREAD)
    {
        kwerrstr("module reader requires a frozen absolute input path");
        return -1;
    }
    return open_file(path, "r");
}


/* Read one module fragment from its borrowed file slot. */
long kread(int fd, void *bytes, long size)
{
    size_t received;
    DVM_FileTicket ticket;
    FVM_Result status;
    FVM_PortFile *file = dvm_runtime_file(fd);
    DVM_FileGate *gate = dvm_runtime_file_gate(fd);
    if (!file || size < 0)
    {
        return -1;
    }
    dvm_file_gate_reserve(gate, &ticket);
    if (!dvm_file_gate_enter_now(gate, &ticket))
    {
        return -1;
    }
    status = fvm_port_file_read(file, bytes, size, &received);
    if (!dvm_file_gate_leave(gate, &ticket))
    {
        fprintf(stderr, "disvm: loader file admission lost\n");
        exit(1);
    }
    if (status != FVM_OK)
    {
        return -1;
    }
    return received;
}


/* EN: Reset the reference scheduler's watchdog after successful stdin.
 * Called by its active controller after managed reentry. Resident execution
 * has no slice watchdog. No parameters, result, allocation or blocking.
 * RU: Сбросить счётчик ссылочного планировщика после успешного stdin.
 * Вызывается активным контроллером после возврата. У закреплённого потока
 * такого счётчика нет. Параметров, результата и выделения памяти нет. */
void dvm_runtime_input_progress(void)
{
    if (native_spawn)
    {
        return;
    }
    progress_epoch++;
    dvm_native_execution()->idle_slices = 0;
    dvm_native_execution()->progress_epoch = progress_epoch;
}


/* EN: Detach an exclusively owned slot under the registry mutex. Borrowers must
 * already be gone (loader ownership or unreachable rooted FD). Retire the gate
 * and submit detached native handles outside the mutex. Return 0 or -1 if absent.
 * RU: Снять принадлежащий вызывающему слот под мьютексом реестра. Заёмщиков
 * уже нет: загрузчик владеет слотом либо FD недостижим. Удалить шлюз и передать
 * handles на закрытие вне мьютекса. Вернуть 0 либо -1 при отсутствии. */
int kclose(int fd)
{
    DVM_FileSlot slot;
    if (!dvm_file_slots_take(fd, 1, &slot))
    {
        return -1;
    }
    if (!dvm_file_gate_destroy(slot.gate))
    {
        fprintf(stderr, "disvm: closing a file with pending native operations\n");
        exit(1);
    }
    dvm_close_submit_resource(slot.job, &slot.resource);
    return 0;
}


/* Module dynamic seeks are not required by ordinary Dis input. */
vlong kseek(int fd, vlong position, int origin)
{
    USED(fd);
    USED(position);
    USED(origin);
    error("native dynamic module seek unsupported");
    return -1;
}


/* Return a diagnostic for the interpreter's %r conversion. */
char *syserr(char *start, char *end, Prog *process)
{
    USED(process);
    return seprint(start, end, "%s", owner_error());
}


/* Reserve the complete native-cell entry frame before publishing its process. */
static Prog *start_module(Module *module)
{
    Import empty;
    Modlink *link;
    Prog initial;
    Prog *process;
    Frame *frame;
    memset(&initial, 0, sizeof(initial));
    memset(&empty, 0, sizeof(empty));
    if (!module->entryt || module->entryt->size < sizeof(Frame))
    {
        error("incompatible legacy module frame; native-cell module required");
    }
    link = linkmod(module, &empty, 1);
    frame = calloc(1, module->entryt->size);
    if (!frame)
    {
        error(exNomem);
    }
    frame->t = module->entryt;
    initial.R.M = link;
    initial.R.MP = link->MP;
    initial.R.PC = module->entry;
    DVM_R.s = &frame;
    newstack(&initial);
    initmem(module->entryt, initial.R.FP);
    process = create_process(nil, link, &initial.R, NULL);
    if (dvm_refcount_release_u(&D2H(link)->ref) != DVM_REFCOUNT_LIVE)
    {
        error(exHeap);
    }
    free(frame);
    return process;
}
