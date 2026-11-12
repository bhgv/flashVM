/* Copyright (c) 2026 bhgv
 * EN: Guard omission controls and array-length operation.
 * RU: Контроли пропуска guard и операция длины массива.
 * Extracted private oracle fragment; assembled only by its Python driver.
 */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
DVM_CellResult __real_dvm_managed_small_three_begin(const void *, size_t, const void *, size_t, void *, size_t, DVM_TripleAccess *);
DVM_CellResult __real_dvm_small_range_access_extend(DVM_TripleAccess *, const void *, size_t);
void *__real_dvm_runtime_array_length_source(const void *);
void __real_dvm_runtime_array_length_value(const void *, void *);
void __real_error(char *);
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void __real_free(void *);


/* Count actual native allocation only inside the borrowed guarded operation. */
void *__wrap_malloc(size_t size)
{
    if (active)
    {
        allocations++;
    }
    return __real_malloc(size);
}


/* Count allocation requests without replacing libc storage or ownership. */
void *__wrap_calloc(size_t count, size_t size)
{
    if (active)
    {
        allocations++;
    }
    return __real_calloc(count, size);
}


/* Count retirement inside the operation; actual free remains untouched. */
void __wrap_free(void *value)
{
    if (active)
    {
        allocations++;
    }
    __real_free(value);
}


/* valid/message are an independent fixture predicate and borrowed diagnostic.
 * Semantic10 terminates before controlled defects can access unguarded fields. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL array length: %s\n", message);
        exit(10);
    }
}


/* Independent canonical boundary-key cardinality for up to three native spans.
 * Geometry is mapped by the fixture, not granted by this numeric calculation. */
static unsigned int union_size(const void *a, const void *b, const void *c)
{
    const void *slots[3] = {a, b, c};
    unsigned int keys[6];
    unsigned int count = 0;
    unsigned int index;
    for (index = 0; index < 3; index++)
    {
        unsigned int edge;
        if (!slots[index])
        {
            continue;
        }
        for (edge = 0; edge < 2; edge++)
        {
            uintptr_t address = (uintptr_t)slots[index]+(edge ? sizeof(void *)-1 : 0);
            unsigned int key = (unsigned int)((address/sizeof(void *))%256);
            unsigned int prior;
            for (prior = 0; prior < count && keys[prior] != key; prior++) { }
            if (prior == count)
            {
                keys[count++] = key;
            }
        }
    }
    return count;
}


/* First claim must exclude reference/output before any source pointer read. */
DVM_CellResult __wrap_dvm_managed_small_three_begin(const void *a, size_t an,
    const void *b, size_t bn, void *d, size_t dn, DVM_TripleAccess *access)
{
    static Inst *index_retry_pc; /* Guest index PC at the pre-effect refusal. */
    static int index_replayed; /* One matching decoded index resumed. */
    int opcode = dvm_native_execution()->arithmetic_retry_opcode;

    static int selection_refused; /* Refuse only the first selected guard capture. */
    if (CONTROL == 0 && !active && opcode == IALT && !selection_refused &&
        a == (uchar *)DVM_R.s + offsetof(Alt, ac) + offsetof(Altc, c))
    {
        selection_refused = 1;
        fprint(1, "PASS ALT selected guard capture refusal\n");
        return DVM_CELL_BUSY;
    }

    if (CONTROL == 0 && !active &&
        ((opcode == IALT && contend_capture(48)) || (opcode == INBALT && contend_capture(49)) ||
         (opcode == ISLICEA && contend_capture(0)) || (opcode == ISLICELA && contend_capture(2)) ||
         (opcode == ICVTCA && contend_capture(6)) || (opcode == ICVTAC && contend_capture(7)) ||
         (opcode == ICVTWC && contend_capture(8)) || (opcode == ICVTLC && contend_capture(9)) ||
         (opcode == ICVTFC && contend_capture(10))))
    {
        return DVM_CELL_BUSY;
    }

    if (CONTROL == 0 && !active && !index_replayed &&
        (opcode == IINDX || opcode == IINDW || opcode == IINDF || opcode == IINDL || opcode == IINDB))
    {
        if (!index_retry_pc)
        {
            index_retry_pc = DVM_R.PC;
            return DVM_CELL_BUSY;
        }
        check(index_retry_pc == DVM_R.PC, "index retries the same decoded instruction");
        index_replayed = 1;
    }

    if (active)
    {
        check(a == source_cell && b == source_cell && d == output_cell &&
            an == sizeof(Array *) && bn == sizeof(Array *) && dn == sizeof(WORD), "initial spans");
        expected_depth = union_size(source_cell, output_cell, NULL);
        if (CONTROL == 1)
        {
            return DVM_CELL_ACQUIRED;
        }
    }
    return __real_dvm_managed_small_three_begin(a, an, b, bn, d, dn, access);
}


/* Actual capture happens under the initial guards, without changing real owners. */
void *__wrap_dvm_runtime_array_length_source(const void *source)
{
    if (active)
    {
        check(source == source_cell && dvm_native_context_object_count(dvm_native_context_current()) ==
            expected_depth && expected_depth, "source read guarded");
    }
    return __real_dvm_runtime_array_length_source(source);
}


/* Discover/claim the field while every original claim remains live. This hook
 * only inspects owned native metadata, never forges guard or Heap headers. */
DVM_CellResult __wrap_dvm_small_range_access_extend(DVM_TripleAccess *access, const void *field, size_t size)
{
    static Inst *retry_pc; /* Post-decode PC while extension is contended. */
    static int replayed; /* The guest retried the original length instruction. */

    if (CONTROL == 0 && !active && !replayed &&
        dvm_native_execution()->arithmetic_retry_opcode == ILENA)
    {
        if (!retry_pc)
        {
            retry_pc = DVM_R.PC;
            return DVM_CELL_BUSY;
        }
        check(retry_pc == DVM_R.PC, "length extension BUSY replays the same PC");
        replayed = 1;
        print("PASS scheduled length extension retry\n");
    }
    if (active)
    {
        check(field == field_cell && size == sizeof(WORD) && access->count == expected_depth &&
            access->held == expected_depth && dvm_native_context_object_count(dvm_native_context_current()) ==
            expected_depth, "continuous source prefix");
        expected_depth = union_size(source_cell, output_cell, field_cell);
        if (CONTROL == 2)
        {
            return DVM_CELL_ACQUIRED;
        }
        if (CONTROL == 4)
        {
            return DVM_CELL_BUSY;
        }
    }
    return __real_dvm_small_range_access_extend(access, field, size);
}


/* Guard depth independently accounts for the discovered field before any load.
 * Omitted extension is caught before reading an unexcluded field. */
void __wrap_dvm_runtime_array_length_value(const void *field, void *destination)
{
    if (active)
    {
        check(field == field_cell && destination == output_cell && expected_depth &&
            dvm_native_context_object_count(dvm_native_context_current()) == expected_depth, "field read guarded");
        if (array && array != H)
        {
            check(dvm_refcount_load_u(&D2H(array)->ref) == 1 &&
                dvm_runtime_root_count(D2H(array)) == 1, "no temporary owners or roots");
        }
        check(!dvm_native_owner_leave(dvm_native_owner_current()), "guards exclude world release");
        if (CONTROL == 3)
        {
            return;
        }
    }
    __real_dvm_runtime_array_length_value(field, destination);
}


/* Real error propagation must follow release of original and extended guards. */
void __wrap_error(char *message)
{
    check(!dvm_native_context_object_count(dvm_native_context_current()), "error after complete unlock");
    __real_error(message);
    abort();
}


/* Invoke the actual opcode through hosted kprotect, with genuine Proc labels. */
static int invoke(void *unused)
{
    (void)unused;
    lena();
    return 1;
}


/* array/source/output globals borrow independently kept-alive fixture storage.
 * Call once, then require unlocked state and zero allocation before returning. */
static int operation(void)
{
    int result;
    allocations = 0;
    DVM_R.s = (void *)source_cell;
    DVM_R.d = output_cell;
    active = 1;
    result = kprotect(invoke, NULL);
    active = 0;
    check(!allocations && !dvm_native_context_object_count(dvm_native_context_current()), "allocation-free interval");
    return result;
}


/* workers is a real service; all arrays/children use actual allocation and GC.
 * Native alias cells borrow a separately rooted owner. Restore REG before actual
 * collector intervals; no fabricated Heap or native-key root enters the GC. */
