/* Copyright (c) 2026 bhgv
 * EN: Real array alias cases and collector cleanup.
 * RU: Настоящие alias-случаи массива и очистка сборщиком.
 * Extracted private oracle fragment; assembled only by its Python driver.
 */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    Type *types[4] = {&Tbyte, &Tword, &Tlong, &Tptr};
    REG saved;
    char message[ERRMAX];
    unsigned char bytes[80];
    unsigned char wanted[80];
    unsigned int kind;
    unsigned int length;
    unsigned int skew;
    unsigned long cases = 0;
    int delta;
    __real_dvm_runtime_root_registry_probe(workers);
    array_collectors = workers;
    check(sizeof(WORD) == sizeof(void *), "native fixture width");
    memcpy(&saved, &DVM_R, sizeof(saved));
    memcpy(message, getup()->env->errstr, sizeof(message));
    for (kind = 0; kind < 4; kind++)
    {
        for (length = 0; length < 32; length++)
        {
            Heap *heap = heaparray(types[kind], (int)length);
            WORD expected = length;
            array = H2D(Array *, heap);
            ptradd(heap);
            field_cell = (unsigned char *)array+offsetof(Array, len);
            for (skew = 0; skew < 8; skew++)
            {
                for (delta = -7; delta <= 7; delta++)
                {
                    memset(bytes, 0xa5, sizeof(bytes));
                    source_cell = bytes+24+skew;
                    output_cell = bytes+24+skew+delta;
                    memcpy((void *)source_cell, &array, sizeof(array));
                    memcpy(wanted, bytes, sizeof(bytes));
                    if (CONTROL == 4)
                    {
                        check(!operation() && !strcmp(getup()->env->errstr, "array length guard refused") &&
                            !memcmp(bytes, wanted, sizeof(bytes)) && dvm_refcount_load_u(&heap->ref) == 1 &&
                            dvm_runtime_root_count(heap) == 1, "extension refusal unchanged");
                        ptrdel(heap);
                        destroy(array);
                        goto finish;
                    }
                    memcpy(wanted+24+skew+delta, &expected, sizeof(expected));
                    check(operation() && !memcmp(bytes, wanted, sizeof(bytes)), "exact length and canaries");
                    cases++;
                }
            }
            source_cell = &array;
            output_cell = &array->len;
            check(operation() && array->len == expected, "field already in initial union");
            ptrdel(heap);
            destroy(array);
        }
    }
    check(cases == 15360, "all real typed array cases");
    for (kind = 0; kind < 2; kind++)
    {
        WORD zero = 0;
        array = kind ? H : NULL;
        field_cell = NULL;
        for (delta = -7; delta <= 7; delta++)
        {
            memset(bytes, 0xa5, sizeof(bytes));
            source_cell = bytes+24;
            output_cell = bytes+24+delta;
            memcpy((void *)source_cell, &array, sizeof(array));
            memcpy(wanted, bytes, sizeof(bytes));
            memcpy(wanted+24+delta, &zero, sizeof(zero));
            check(operation() && !memcmp(bytes, wanted, sizeof(bytes)), "nil value gives zero");
        }
    }
    {
        void *invalid[3] = {NULL, H, (void *)((uintptr)-1-2)};
        Array *valid = H;
        WORD output = 0x1234;
        for (kind = 0; kind < 3; kind++)
        {
            DVM_R.s = invalid[kind];
            DVM_R.d = &output;
            check(!kprotect(invoke, NULL) && output == 0x1234 &&
                !strcmp(getup()->env->errstr, exBounds), "bad source geometry");
            DVM_R.s = &valid;
            DVM_R.d = invalid[kind];
            check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exBounds), "bad output geometry");
        }
    }
    {
        Heap *heap = heaparray(&Tptr, 2);
        String *child;
        WORD output = -1;
        DVM_HeapOwner *owner = dvm_native_owner_current();
        array = H2D(Array *, heap);
        ptradd(heap);
        child = c2string("kept", 4);
        memcpy(array->data, &child, sizeof(child)); /* Transfer child initial owner before GC. */
        source_cell = &array;
        output_cell = &output;
        field_cell = &array->len;
        check(operation() && output == 2, "mapped-array field snapshot");
        memcpy(&DVM_R, &saved, sizeof(saved));
        check(dvm_native_owner_leave(owner), "leave after guard release");
        for (kind = 0; kind < 32; kind++)
        {
            check(dvm_gc_workers_request(workers, DVM_GC_CYCLES) && dvm_gc_workers_wait_idle(workers), "actual array Cycles");
        }
        check(dvm_native_owner_enter(owner) && array->len == 2 && child->len == 4 &&
            !memcmp(child->Sascii, "kept", 4) && dvm_refcount_load_u(&heap->ref) == 1 &&
            dvm_runtime_root_count(heap) == 1, "caller array and child survive");
        ptrdel(heap);
        destroy(array);
    }
finish:
    array = NULL;
    memcpy(&DVM_R, &saved, sizeof(saved));
    memcpy(getup()->env->errstr, message, sizeof(message));
    {
        DVM_HeapOwner *owner = dvm_native_owner_current();
        check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers, DVM_GC_FAST) &&
            dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner), "actual final Fast");
    }
    print("PASS array length probe:15360 real alias cases or explicit extension refusal\n");
}
