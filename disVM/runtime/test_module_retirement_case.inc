/* Copyright (c) 2026 bhgv
 * EN: Private module construction, consumed owner lifecycle and actual JIT tail probe.
 * RU: Частное создание модуля, снятие владельцев и настоящий JIT tail-entry.
 */
static _Thread_local Modlink *tail_link;  /* Parent-owned link passed synchronously into the target tail. */
static _Thread_local Module *tail_lookup; /* Optional lookup owner consumed by the same callback. */
static _Thread_local Type *tail_type;     /* Independent Type creator consumed by the same callback. */
static _Thread_local int tail_pending;    /* One immediate target call, never another resident's work. */


/* [GC:] EN: Consume privately owned link, optional lookup owner and Type creator.
 * Keep native peers admitted until all target heap/metadata/code frees complete.
 * Called either from ordinary C setup or after an actual JIT tail entry has left
 * its disposable mapping. Errors terminate this private diagnostic.
 * RU: Снять частные link, необязательную ссылку поиска и ссылку создателя Type.
 * Удержать соседей до освобождения кучи/метаданных/кода. Вызывается из C либо
 * после выхода настоящего JIT tail-entry из собственной mapping. Ошибки
 * завершают частную проверку.
 */
static void retire_fixture(Modlink *link, Module *lookup_owner, Type *type)
{
    require(dvm_managed_owner_capture(link, &ticket) && dvm_refcount_drop_nonfinal_u(&D2H(link)->ref),
            "ticket owns the last Modlink reference");
    FVM_PortThread *observer = fvm_port_thread_start(hold_module_reader, dvm_gc_runtime.world);
    require(observer != NULL, "native module reader starts");
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "parent native roots published");
    unsigned long long deadline = milliseconds() + 5000;
    int success = 0;
    while (milliseconds() < deadline)
    {
        pthread_mutex_lock(&sync);
        if (reader_wait_seen && !reader_release)
        {
            reader_release = 1;
            pthread_cond_broadcast(&changed);
        }
        if (later_held && later_passes >= 8)
        {
            later_release = 1;
            pthread_cond_broadcast(&changed);
        }
        int drop_lookup = lookup_owner && (freed & 3) == 3;
        success = freed == expected_mask && metadata_freed && !expired;
        pthread_mutex_unlock(&sync);
        if (drop_lookup)
        {
            dvm_runtime_gc_native_enter(native);
            require(!metadata_freed && dvm_refcount_load_i(&lookup_owner->ref) == 1,
                    "nonfinal link release preserves lookup owner");
            unload(lookup_owner);
            lookup_owner = NULL;
            native = dvm_runtime_gc_native_try_leave();
            require(native > 0, "lookup owner retirement publishes parent roots");
        }
        if (success)
        {
            break;
        }
        fvm_port_thread_sleep_us(1000);
    }
    require(success, "ordinary module retirement still requires heap quiescence");
    require(CONTROL != 2 || root_refused, "MP root refusal was not exercised");
    require((CONTROL != 3 && CONTROL != 7) || claim_refused, "competing retain was not exercised");
    fvm_port_thread_join(observer);
    dvm_runtime_gc_native_enter(native);
    if (type)
    {
        require(dvm_refcount_load_i(&type->ref) == 1, "Module and both MPs released their Type owners");
        freetype(type);
    }
    pthread_mutex_lock(&sync);
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    fprintf(
        stderr,
        "PASS shared module retirement heaps=%u metadata=1 reader_epochs=2 lookup=%d root_refusal=%d claim_refusal=%d entry_refusal=%d code=%d\n",
        EmptyModule ? 1U : 4U,
        CONTROL == 1,
        root_refused,
        claim_refused,
        entry_refused,
        code_unmapped);
}


/* [GC:] EN: Intercept only the synchronous target RET bridge. Run the real
 * retirement fixture after generated code tail-jumps here, so its mapping may
 * disappear before this C callback returns. Ordinary application dispatch stays
 * real. registers/operation/fast otherwise forward unchanged; do not use fast
 * after retirement because it may point into the reclaimed mapping.
 * RU: Перехватить только синхронный RET-bridge цели. Выполнить настоящее
 * освобождение после tail-перехода сюда: mapping может исчезнуть до возврата C.
 * Остальной dispatch приложения настоящий. Иначе передать registers/operation/
 * fast без изменения; после освобождения fast не использовать.
 */
int __wrap_cjitexec(REG *registers, int operation, int (*fast)(REG *))
{
    if (!tail_pending)
    {
        return __real_cjitexec(registers, operation, fast);
    }
    require(registers == &DVM_R && operation == IRET, "actual target tail passes decoded scalar arguments");
    tail_pending = 0;
    retire_fixture(tail_link, tail_lookup, tail_type);
    tail_link = NULL;
    tail_lookup = NULL;
    tail_type = NULL;
    require(code_unmapped, "target tail callback must reclaim its own original mapping");
    return 1;
}


/* [GC:] EN: Construct a real empty or mapped builtin Module and optional JIT.
 * Mapped cases keep an independent Type creator and shared string; lookup cases
 * acquire another Module owner. Observe target heap/metadata/code destruction
 * before releasing peers. frame stays production-owned; setup owners pass once
 * to retire_fixture directly or through the actual emitted tail entry.
 * RU: Создать настоящий пустой/размеченный builtin Module и необязательный JIT.
 * Размеченные случаи сохраняют ссылку создателя Type и строку; поиск добавляет
 * владельца Module. Увидеть освобождение кучи/метаданных/кода до выхода соседей.
 * frame принадлежит runtime; владельцы передаются один раз в retire_fixture
 * напрямую либо через настоящий сгенерированный tail-entry.
 */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    pthread_mutex_lock(&sync);
    while (admitted != 2)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
    Runtab empty[1] = {{0}};
    Module *module = builtinmod((char *)ModuleName, empty, 0);
    require(module != NULL, "actual builtin Module");
    Type *type = NULL;
    String *child = H;
    if (!EmptyModule)
    {
        uchar map = 0x80;
        type = dtype(freeheap, 2 * sizeof(void *), &map, 1);
        require(type && dvm_refcount_retain_i(&type->ref) == DVM_REFCOUNT_LIVE, "independent Type owner");
        module->type = mallocz(sizeof(Type *), 1);
        require(module->type != NULL, "actual Module type vector");
        module->type[0] = type;
        module->ntype = 1;
        module->origmp = H2D(uchar *, heapz(type));
        child = newstring(32);
        memcpy(module->origmp, &child, sizeof(child));
        Setmark(D2H(child));
    }
    if (CONTROL == 9)
    {
        module->prog = mallocz(sizeof(Inst), 1);
        require(module->prog != NULL, "actual compiler input allocation");
        module->prog[0].op = IRET;
        module->nprog = 1;
        require(compile(module, module->nprog, NULL) && dvm_jit_load(module) != NULL,
                "actual native leaf compiler publishes target code");
        Jit *jit = dvm_jit_load(module);
        require(jit->code && jit->size > 0 && jit->freecode, "target owns executable mapping and destructor");
        code_bytes = (size_t)jit->size;
        __atomic_store_n(&code_identity, jit->code, __ATOMIC_RELEASE);
    }
    Module *lookup_owner = CONTROL == 1 ? lookmod((char *)ModuleName) : NULL;
    Import imports[1] = {{0}};
    Modlink *link = linkmod(module, imports, 1);
    require(link != H && (EmptyModule ? link->MP == H : link->MP != H && link->MP != module->origmp),
            "actual MP clone or genuinely empty module");
    pthread_mutex_lock(&sync);
    __atomic_store_n(&target_module, module, __ATOMIC_RELEASE);
    targets[0] = D2H(link);
    expected_mask = EmptyModule ? 1 : 15;
    if (!EmptyModule)
    {
        targets[1] = D2H(link->MP);
        targets[2] = D2H(module->origmp);
        targets[3] = D2H(child);
    }
    for (unsigned index = 0; index < 4; index++)
    {
        if (!targets[index])
        {
            continue;
        }
        keys[index] = dvm_heap_pool_key(heapmem, targets[index]);
        require(keys[index].generation != 0, "real target allocation generation");
    }
    pthread_mutex_unlock(&sync);
    if (CONTROL == 9)
    {
        int (*dispatch)(REG *) = dvm_jit_load(module)->dispatch[IRET];
        require(dispatch != NULL, "actual target complete tail entry exists");
        tail_link = link;
        tail_lookup = lookup_owner;
        tail_type = type;
        tail_pending = 1;
        require(dispatch(&DVM_R) == 1 && code_unmapped && !tail_pending,
                "native tail returns to C after its code mapping is gone");
        fprintf(stderr, "PASS actual JIT tail returned after original mapping unmap\n");
    }
    else
    {
        retire_fixture(link, lookup_owner, type);
    }
}
