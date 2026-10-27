/* Shared implementation fragment: REG/Jit and native_instructions are supplied
 * by the runtime translation unit or the focused C99 fixture. */


/* Apply the NC64 native policy to either a direct entry or a complete tail entry.
 * operation is a decoded opcode, fast a non-null borrowed function and registers
 * the live borrowed VM state. Return the leaf result; count only nonzero success.
 * Classic fixed-width tables decline before calling fast. No module/Jit pointer
 * is retained across callbacks. Allocation, errors and blocking follow the
 * selected callback contract; a throwing callback never increments success. */
int dvm_runtime_fast(int operation, int (*fast)(REG *), REG *registers)
{
    int accepted;
    if (operation == ICASE || operation == ICASEL || operation == IGOTO)
    {
        return 0;
    }
    accepted = fast(registers);
    if (accepted)
    {
        native_instructions++;
    }
    return accepted;
}


/* Borrow one already-selected Jit entry and apply the same policy as cjitexec.
 * jit/operation/registers are validated by decoded dispatch; return acceptance.
 * The entry is captured before the call and no Jit storage is read afterward. */
int dvm_runtime_leaf(Jit *jit, int operation, REG *registers)
{
    return dvm_runtime_fast(operation, jit->entry[operation], registers);
}
