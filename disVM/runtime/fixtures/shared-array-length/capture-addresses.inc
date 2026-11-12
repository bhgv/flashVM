/* Copyright (c) 2026 bhgv
 * EN: Address lifetime through real collector passes.
 * RU: Время жизни адресов при настоящих проходах сборщика.
 * Extracted private oracle fragment; assembled only by its Python driver.
 */
static void collect_before_address_use(int opcode)
{
    static unsigned int checked; /* One full collection per actual index opcode. */
    unsigned int bit;
    int released;

    switch (opcode)
    {
    case IINDX: bit = 1; break;
    case IINDW: bit = 2; break;
    case IINDF: bit = 4; break;
    case IINDL: bit = 8; break;
    case IINDB: bit = 16; break;
    default: return;
    }
    if (checked & bit)
    {
        return;
    }
    released = dvm_runtime_gc_native_try_leave();
    if (released != 1 || !array_collectors ||
        !dvm_gc_workers_request(array_collectors, DVM_GC_CYCLES) ||
        !dvm_gc_workers_wait_idle(array_collectors))
    {
        fprint(2, "FAIL cannot collect between array address and use\n");
        exit(10);
    }
    dvm_runtime_gc_native_enter(released);
    checked |= bit;
    if ((checked & 29) == 29)
    {
        print("PASS four interior addresses survive Cycles before use\n");
    }
}


/* Check each real address publication, independently of operand capture. */
const char *__wrap_dvm_runtime_bytes_publish_opcode(int opcode, const void *source, void *destination, size_t size)
{
    const char *problem;
    address_publication = 1;
    problem = __real_dvm_runtime_bytes_publish_opcode(opcode, source, destination, size);
    if (!problem && address_publication != 2)
    {
        fprint(2, "FAIL address publication skipped the destination guard\n");
        exit(10);
    }
    address_publication = 0;
    if (CONTROL == 0 && !problem && dvm_native_execution()->arithmetic_retry_opcode == opcode)
    {
        collect_before_address_use(opcode);
    }
    return problem;
}


static int index_reported; /* Emit one actual route marker per fixture process. */
const char *__real_dvm_runtime_array_index_operands(const void *, const void *, void *, void *, int *);


/* Verify that real C and compiled guests both enter the guarded index helper. */
const char *__wrap_dvm_runtime_array_index_operands(const void *source, const void *index,
    void *array_snapshot, void *index_snapshot, int *replay)
{
    const char *problem = __real_dvm_runtime_array_index_operands(source, index,
        array_snapshot, index_snapshot, replay);

    if (*replay)
    {
        print("PASS scheduled index retry\n");
    }
    if (!problem && !*replay && !index_reported)
    {
        index_reported = 1;
        print("PASS guarded array index route\n");
    }
    return problem;
}


static Array *array; /* Actual independently rooted fixture owner, or nil/H. */
static const void *source_cell; /* Complete borrowed reference slot. */
static void *output_cell; /* Complete borrowed writable WORD. */
static const void *field_cell; /* Expected live field or NULL for nil/H. */
