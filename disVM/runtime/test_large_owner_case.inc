/* Copyright (c) 2026 bhgv
 * [GC:] EN: Construct real wide owners and require completion before peer release.
 * RU: Создать настоящих широких владельцев и завершить GC до ухода соседей.
 */


/* [GC:] EN: Own private construction through target publication. frame is
 * forwarded unchanged; Type creators stay independently owned until physical
 * frees finish. No fake collector result or unregistered borrowed owner escapes.
 * RU: Владеть построением до публикации target. frame передаётся без изменения;
 * создатели Type удерживаются независимо до завершения физических free.
 * Подмена ответов GC и выдача неучтённых заимствованных ссылок не допускаются. */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    pthread_mutex_lock(&sync);
    while (admitted != 2)
    {
        pthread_cond_wait(&changed, &sync);
    }
    pthread_mutex_unlock(&sync);
    Type *unknown_type = dtype(unknown_destructor, sizeof(void *), NULL, 0);
    require(unknown_type != NULL, "actual unsupported descriptor");
    Heap *retained = heapz(unknown_type);
    pthread_mutex_lock(&sync);
    unknown = retained;
    unknown_key = dvm_heap_pool_key(heapmem, retained);
    pthread_mutex_unlock(&sync);
    require(dvm_runtime_rc_defer(H2D(void *, retained)), "real retained owner publication");
    uchar map[(Slots + 7) / 8];
    memset(map, 0, sizeof(map));
    for (unsigned index = 0; index < Slots; index++)
    {
        map[index / 8] |= 0x80U >> (index % 8);
    }
    uchar pointer_map = 0x80;
    Type *type = IsArray
                     ? dtype(freeheap, sizeof(void *), &pointer_map, 1)
                     : dtype(freeheap, Slots * sizeof(void *), Case == 26 ? NULL : map, Case == 26 ? 0 : sizeof(map));
    require(type != NULL, "actual wide mapped descriptor");
    Heap *container;
    void **fields;
    Heap *local_children[Children + 1];
    unsigned local_occurrences[Children + 1] = {0};
    unsigned used = 0;
    if (IsList)
    {
        container = nheap(offsetof(List, data) + Slots * sizeof(void *));
        require(dvm_refcount_retain_i(&Tlist.ref) == DVM_REFCOUNT_LIVE &&
                    (Case == 24 || dvm_refcount_retain_i(&type->ref) == DVM_REFCOUNT_LIVE),
                "actual parent list descriptor owners");
        container->t = &Tlist;
        List *list = H2D(List *, container);
        list->t = Case == 24 ? NULL : type;
        list->tail = H;
        if (!NoChildren)
        {
            Heap *tail_heap = nheap(sizeof(List));
            require(dvm_refcount_retain_i(&Tlist.ref) == DVM_REFCOUNT_LIVE, "actual tail descriptor owner");
            tail_heap->t = &Tlist;
            List *tail = H2D(List *, tail_heap);
            tail->tail = H;
            tail->t = NULL;
            list->tail = tail;
            Setmark(tail_heap);
            local_children[used] = tail_heap;
            local_occurrences[used++] = 1;
        }
        fields = (void **)list->data;
    }
    else if (IsArray)
    {
        container = nheap(sizeof(Array) + Slots * sizeof(void *));
        require(dvm_refcount_retain_i(&Tarray.ref) == DVM_REFCOUNT_LIVE &&
                    dvm_refcount_retain_i(&type->ref) == DVM_REFCOUNT_LIVE,
                "actual array descriptor owners");
        container->t = &Tarray;
        Array *array = H2D(Array *, container);
        array->root = H;
        array->len = Case == 22 || Case == 28 ? 0 : Slots;
        array->t = type;
        array->data = (uchar *)(array + 1);
        fields = (void **)array->data;
    }
    else
    {
        container = heapz(type);
        fields = H2D(void **, container);
    }
    for (unsigned index = 0; index < Slots; index++)
    {
        fields[index] = H;
        if (NoChildren || (Case == 1 && index != Slots - 1))
        {
            continue;
        }
        if (Case == 6 && index)
        {
            require(dvm_refcount_retain_u(&local_children[0]->ref) == DVM_REFCOUNT_LIVE,
                    "actual repeated mapped owner");
            fields[index] = H2D(void *, local_children[0]);
            local_occurrences[0]++;
        }
        else
        {
            String *child = newstring(8);
            fields[index] = child;
            local_children[used] = D2H(child);
            local_occurrences[used++] = 1;
            Setmark(D2H(child));
        }
    }
    require(used == Children, "actual unique child count");
    Setmark(container);
    pthread_mutex_lock(&sync);
    target = container;
    target_key = dvm_heap_pool_key(heapmem, container);
    for (unsigned index = 0; index < Children; index++)
    {
        children[index] = local_children[index];
        child_keys[index] = dvm_heap_pool_key(heapmem, children[index]);
        occurrences[index] = local_occurrences[index];
    }
    pthread_mutex_unlock(&sync);
    require(dvm_runtime_rc_defer(H2D(void *, container)), "transfer actual sole parent owner");
    int native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "parent native frame inactive during shared collection");
    unsigned long long deadline = milliseconds() + 10000;
    int done = 0;
    while (milliseconds() < deadline)
    {
        pthread_mutex_lock(&sync);
        if (epochs_held && held_passes >= 8)
        {
            require(!target_freed && !freed, "all targets survive the actual old-reader cutoff");
            epochs_held = 0;
        }
        if (Assist && !released && refusals >= 8)
        {
            released = 1;
            pthread_cond_broadcast(&changed);
        }
        done = target_freed && freed == Children && !expired;
        pthread_mutex_unlock(&sync);
        if (done)
        {
            break;
        }
        fvm_port_thread_sleep_us(1000);
    }
    require(!Assist || unknown_calls, "quiescent cleanup must release shared packet pressure");
    require(done, "supported wide owner must free beside admitted peers");
    dvm_runtime_gc_native_enter(native);
    require(dvm_refcount_load_i(&type->ref) == 1, "all parent element/type descriptor owners released");
    freetype(type);
    pthread_mutex_lock(&sync);
    require((Fault ? refusals >= 8 && failure_passes >= 8 : !refusals) &&
                races == (Race || EmptyRace || EligibilityRace),
            "actual pressure responses and competing last-claim evidence");
    require(!vector && vectors == vector_frees && (IsArray || Owners <= 512 ? !vectors : vectors != 0),
            "temporary vector cardinality and balanced frees");
    require(Assist || (!unknown_calls && !unknown_freed && dvm_runtime_root_count(unknown) == 1 &&
                       dvm_refcount_load_u(&unknown->ref) == 1),
            "unsupported callback stays rooted until peers leave");
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    native = dvm_runtime_gc_native_try_leave();
    require(native > 0, "parent inactive for retained unknown finalizer");
    deadline = milliseconds() + 5000;
    int retained_done = 0;
    while (milliseconds() < deadline)
    {
        pthread_mutex_lock(&sync);
        retained_done = unknown_calls == 1 && unknown_freed == 1;
        pthread_mutex_unlock(&sync);
        if (retained_done)
        {
            break;
        }
        fvm_port_thread_sleep_us(1000);
    }
    require(retained_done, "actual unknown callback and physical free after peer release");
    dvm_runtime_gc_native_enter(native);
    require(dvm_refcount_load_i(&unknown_type->ref) == 1, "unknown descriptor returns to its creator");
    freetype(unknown_type);
    fprintf(stderr,
            "PASS large owner release case=%d slots=%d owners=%d children=%d faults=%u races=%u\n",
            Case,
            Slots,
            Owners,
            Children,
            refusals,
            races);
}
