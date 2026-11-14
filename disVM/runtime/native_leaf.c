/* Copyright (c) 2026 bhgv
 * EN: Shared native callback policy; REG/Jit/counter come from runtime or fixture.
 * RU: Общая политика native callback; REG/Jit/счётчик задаёт runtime либо тест.
 */


/* Apply the NC64 native policy to either a direct entry or a complete tail entry.
 * operation is a decoded opcode, fast a non-null borrowed function and registers
 * the live borrowed VM state. Return the leaf result; count only nonzero success.
 * Classic fixed-width tables decline before calling fast. No module/Jit pointer
 * is retained across callbacks. Allocation, errors and blocking follow the
 * selected callback contract; a throwing callback never increments success.
 * RU: Выполнить fast(registers) для разрешённого operation, вернуть его результат.
 * Учитывать только ненулевой успех атомарно; callback управляет ошибками/ожиданием. */
int dvm_runtime_fast(int operation, int (*fast)(REG *), REG *registers)
{
    int accepted;
    if (operation == ICASE || operation == ICASEL || operation == IGOTO)
    {
        return 0;
    }
#ifdef DVM_RUNTIME_JIT_QUARANTINE
    dvm_jit_native_begin();
#endif
    accepted = fast(registers);
#ifdef DVM_RUNTIME_JIT_QUARANTINE
    dvm_jit_native_end();
#endif
    if (accepted)
    {
        __atomic_fetch_add(&native_instructions, 1, __ATOMIC_RELAXED);
    }
    return accepted;
}


/* Borrow one already-selected Jit entry and apply the same policy as cjitexec.
 * jit/operation/registers are validated by decoded dispatch; return acceptance.
 * The entry is captured before the call and no Jit storage is read afterward.
 * RU: Заимствовать entry из jit для operation/registers; вернуть принятие.
 * После callback память jit больше не читается. */
int dvm_runtime_leaf(Jit *jit, int operation, REG *registers)
{
    return dvm_runtime_fast(operation, jit->entry[operation], registers);
}
