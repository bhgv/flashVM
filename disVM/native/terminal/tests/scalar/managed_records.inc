/* Copyright (c) 2026 bhgv
 * EN: Check bounded managed value records without a VM or a transport.
 * RU: Проверить ограниченные записи управляемых значений без VM и транспорта.
 */


/* EN: Round-trip nested strings and nil, then reject malformed UTF-8 and wrong types.
 * RU: Передать вложенные строки и nil, затем отклонить неверный UTF-8 и чужие типы. */
static int managed_records(void)
{
    const unsigned char text[] = {0xd0, 0x96, 0, 0xf0, 0x9f, 0x8c, 0x8d};
    const unsigned char bad_utf8[] = {0xed, 0xa0, 0x80};
    unsigned char children[128], payload[192], frame[256];
    size_t one, two, array, extra, size;
    FVM_SRecord decoded;
    const unsigned long tag = 89;
    if (fvm_srecord_write(STYX_MANAGED_STRING_TAG, text, sizeof(text), children, sizeof(children), &one) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_NIL_TAG, NULL, 0, children + one, sizeof(children) - one, &two) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, children, one + two, payload, sizeof(payload), &array) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_STRING_TAG, NULL, 0, payload + array, sizeof(payload) - array, &extra) != FVM_S_OK ||
        !styxmanaged_list_write(tag, "(a[s]s)", payload, array + extra, frame, sizeof(frame), &size) ||
        !styxmanaged_list_read(tag, "(a[s]s)", frame, size, &decoded) ||
        decoded.size != array + extra || memcmp(decoded.payload, payload, decoded.size) ||
        styxmanaged_list_read(tag, "(a[s]s)", frame, size - 1, &decoded) ||
        styxmanaged_list_read(tag, "(a[b]s)", frame, size, &decoded) ||
        styxmanaged_list_read(tag, "(a[s])", frame, size, &decoded) ||
        styxmanaged_list_read(tag, "(a[s]r[])", frame, size, &decoded))
    {
        return 0;
    }
    if (fvm_srecord_write(STYX_MANAGED_STRING_TAG, bad_utf8, sizeof(bad_utf8), children, sizeof(children), &one) != FVM_S_OK ||
        fvm_srecord_write(tag, children, one, frame, sizeof(frame), &size) != FVM_S_OK)
    {
        return 0;
    }
    const size_t before = size;
    return !styxmanaged_list_read(tag, "(s)", frame, size, &decoded) &&
        !styxmanaged_list_write(tag, "(s)", children, one, frame, sizeof(frame), &size) &&
        size == before;
}



/* EN: Check root positions for packed bytes, nested tuples and managed references.
 * RU: Проверить позиции корней рядом с упакованными байтами и во вложенном кортеже. */
static int managed_layout(void)
{
    const char shape[] = "(ys(a[b]y)r[ib])";
    const size_t offsets[] = {0, 8, 16, 24, 32};
    StyxScalarLayout layout;
    return !styxscalar_layout(shape, &layout) && styxmanaged_layout(shape, &layout) &&
        layout.size == 40 && layout.count == 5 && layout.pointers == 0x16 &&
        !memcmp(layout.offset, offsets, sizeof(offsets));
}
