/* NC64-only table adapters: compiler cells are eight bytes, unlike classic Dis rows. */


/* Read one unaligned NC64 table cell at a previously range-checked native address.
 * address is borrowed module data. Returns a signed cell without changing the table. */
static WORD nc64_table_word(uintptr address)
{
    WORD value;
    memmove(&value, (void *)address, sizeof(value));
    return value;
}


/* Resolve an ordinary Dis instruction index in the current module.
 * destination is a full NC64 cell. Returns a checked PC or raises a VM error;
 * relocated legacy native-PC modules are not supported by this standalone host. */
static Inst *nc64_table_pc(WORD destination)
{
    Modlink *module = DVM_R.M;
    uintptr base;
    uintptr offset;
    if (module == nil || module == H || module->compiled || module->m == nil || module->m == H)
    {
        error(exModule);
    }
    if (destination < 0 || destination >= module->m->nprog || module->m->prog == nil || module->m->prog == H)
    {
        error(exBounds);
    }
    base = (uintptr)module->m->prog;
    if ((uvlong)destination > (uintptr)-1 / sizeof(Inst))
    {
        error(exBounds);
    }
    offset = (uintptr)destination * sizeof(Inst);
    if (base > (uintptr)-1 - offset)
    {
        error(exBounds);
    }
    return (Inst *)(base + offset);
}


/* Search native-cell numeric rows without changing the compiler's private NC64 ABI.
 * wide selects big tables (two header/padded destination cells) versus int tables.
 * Reads borrowed slots and commits only PC after count/address/destination validation. */
static void nc64_numeric_case(int wide)
{
    uintptr table = (uintptr)DVM_R.d;
    uintptr header = (wide ? 2 : 1) * sizeof(WORD);
    uintptr stride = (wide ? 4 : 3) * sizeof(WORD);
    uintptr span;
    uintptr rows;
    uintptr row;
    WORD count;
    WORD destination;
    LONG value;
    LONG low;
    LONG high;
    int remaining;
    int half;
    if (!table || DVM_R.d == H || table > (uintptr)-1 - (sizeof(WORD) - 1))
    {
        error(exBounds);
    }
    count = nc64_table_word(table);
    if (count < 0 || count > (0x7fffffff - header) / stride)
    {
        error(exBounds);
    }
    span = header + (uintptr)count * stride;
    if (table > (uintptr)-1 - span - (sizeof(WORD) - 1))
    {
        error(exBounds);
    }
    memmove(&value, DVM_R.s, sizeof(value));
    destination = nc64_table_word(table + span);
    rows = table + header;
    remaining = (int)count;
    while (remaining > 0)
    {
        half = remaining >> 1;
        row = rows + (uintptr)half * stride;
        memmove(&low, (void *)row, sizeof(low));
        memmove(&high, (void *)(row + sizeof(LONG)), sizeof(high));
        if (value < low)
        {
            remaining = half;
        }
        else if (value >= high)
        {
            rows = row + stride;
            remaining -= half + 1;
        }
        else
        {
            destination = nc64_table_word(row + 2 * sizeof(LONG));
            break;
        }
    }
    DVM_R.PC = nc64_table_pc(destination);
}


/* Execute an NC64 integer case; all cells and result PC are validated by the helper. */
OP(icase)
{
    nc64_numeric_case(0);
}


/* Execute an NC64 big case, retaining its padded eight-byte-cell layout. */
OP(casel)
{
    nc64_numeric_case(1);
}


/* Execute an indexed NC64 jump using full-width count and destination cells.
 * VM operands are borrowed; malformed indices/addresses/PCs raise before PC changes. */
OP(igoto)
{
    uintptr table = (uintptr)DVM_R.d;
    uintptr offset;
    WORD index;
    WORD count;
    WORD destination;
    memmove(&index, DVM_R.s, sizeof(index));
    if (table < sizeof(WORD) || DVM_R.d == H || index < 0)
    {
        error(exBounds);
    }
    count = nc64_table_word(table - sizeof(WORD));
    if (count <= 0 || index >= count || (uvlong)index > (uintptr)-1 / sizeof(WORD))
    {
        error(exBounds);
    }
    offset = (uintptr)index * sizeof(WORD);
    if (table > (uintptr)-1 - offset || table + offset > (uintptr)-1 - (sizeof(WORD) - 1))
    {
        error(exBounds);
    }
    destination = nc64_table_word(table + offset);
    DVM_R.PC = nc64_table_pc(destination);
}
