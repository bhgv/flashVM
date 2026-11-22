/* Copyright (c) 2026 bhgv
 * EN: Stage field deltas and commit only the touched lanes of each original ADT.
 * RU: Подготовить дельты и применить только изменившиеся поля исходных ADT.
 */


/* EN: Build a rooted private ADT with selected slots populated from the decoded frame.
 * RU: Создать частный ADT с корнем и заполнить выбранные слоты из декодированного кадра. */
static int stage_delta(const char *type, const StyxFieldDelta *delta, unsigned char *wire,
    DVM_StyxValueFrame *stage, size_t index)
{
    char fields[STYX_SHAPE_MAX + 1];
    StyxScalarLayout layout;
    size_t used;
    DVM_StyxValueFrame source = {0};
    if (!styxmanaged_ref_fields(type, fields) || !styxmanaged_layout(fields, &layout) ||
        fvm_srecord_write(STYX_MANAGED_REF_TAG, delta->fields.payload, delta->fields.size,
            wire, STYX_MANAGED_LIST_LIMIT, &used) != FVM_S_OK ||
        !dvm_styx_value_frame_read(STYX_MANAGED_REF_TAG, delta->schema, wire, used, &source))
    {
        return 0;
    }
    unsigned char map = 0;
    for (size_t i = 0; i < layout.count; i++)
    {
        if (layout.pointers & (1U << i))
        {
            map |= 0x80U >> (layout.offset[i] / sizeof(void *));
        }
    }
    RefAllocation allocation = {.type = dtype(freeheap, (layout.size + 7) & ~(size_t)7, &map, !!map)};
    int allocated = allocation.type && kprotect(allocate_ref, &allocation);
    freetype(allocation.type);
    if (!allocated)
    {
        dvm_styx_value_frame_clear(&source);
        return 0;
    }
    unsigned char *box = allocation.object;
    for (size_t i = 0, j = 0; i < layout.count; i++)
    {
        if (delta->mask & (1U << i))
        {
            void *slot = (unsigned char *)source.words + source.layout.offset[j];
            memcpy(box + layout.offset[i], slot, layout.width[i]);
            if (source.roots & (1U << j))
            {
                void *value;
                memcpy(&value, slot, sizeof(value));
                ptrdel(D2H(value));
            }
            j++;
        }
    }
    memcpy((unsigned char *)stage->words + stage->layout.offset[index], &box, sizeof(box));
    return 1;
}


/* EN: Register a temporary root for a staged box before allocating further values.
 * RU: Зарегистрировать временный корень ADT перед дальнейшими выделениями. */
static int root_delta(void *box)
{
    ptradd(D2H(box));
    Setmark(D2H(box));
    return 1;
}


/* EN/RU: See managed_ref.h. / См. managed_ref.h. */
int dvm_styx_ref_delta_commit(const char *arguments, const void *slots, const StyxFieldDelta *changes,
    size_t count, void *destination, DVM_StyxValueFrame *result)
{
    char refs[STYX_SHAPE_MAX + 1], selected[STYX_SHAPE_MAX + 1] = "(";
    void *objects[STYX_CALL_VALUES] = {0}, *targets[STYX_CALL_VALUES];
    const char *types[STYX_CALL_VALUES];
    unsigned masks[STYX_CALL_VALUES];
    if (!dvm_styx_ref_slots(arguments, slots, refs, objects))
    {
        return 0;
    }
    size_t length = 1, seen = 0;
    for (const char *type = refs + 1; *type != ')'; type = styxshape_end(type))
    {
        types[seen++] = type;
    }
    for (size_t i = 0; i < count; i++)
    {
        unsigned object = changes[i].object;
        if (object >= seen || !objects[object] || objects[object] == H)
        {
            return 0;
        }
        for (size_t j = 0; j < object; j++)
        {
            if (objects[j] == objects[object])
            {
                return 0;
            }
        }
        const char *type = types[object];
        size_t n = (size_t)(styxshape_end(type) - type);
        memcpy(selected + length, type, n);
        length += n;
        targets[i] = objects[object];
        masks[i] = changes[i].mask;
    }
    selected[length++] = ')';
    selected[length] = 0;
    DVM_StyxValueFrame stage = {0};
    if (!styxmanaged_layout(selected, &stage.layout))
    {
        return 0;
    }
    unsigned char *wire = count ? malloc(STYX_MANAGED_LIST_LIMIT) : NULL;
    int valid = !count || wire;
    for (size_t i = 0; valid && i < count; i++)
    {
        valid = stage_delta(types[changes[i].object], &changes[i], wire, &stage, i);
        if (valid)
        {
            void *box;
            memcpy(&box, (unsigned char *)stage.words + stage.layout.offset[i], sizeof(box));
            valid = kprotect(root_delta, box);
            if (valid)
            {
                stage.roots |= 1U << i;
            }
        }
    }
    valid = valid && ref_commit(selected, targets, &stage, masks, destination, result);
    dvm_styx_value_frame_clear(&stage);
    free(wire);
    return valid;
}
