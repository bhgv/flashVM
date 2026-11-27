/* Copyright (c) 2026 bhgv
 * EN: Derive complete catalogue names and wire offers from one rooted Dis instance.
 * RU: Получить полные имена и предложения протокола из одного укоренённого экземпляра Dis.
 */


typedef struct LoadedCatalogueEntry
{
    const Link *export;  /* Borrowed compiled export, pinned by the caller's anchor. */
    size_t name_length;  /* Verified name length in bytes. */
    StyxCallShape shape; /* Shape validated by the same gateway used for binding. */
} LoadedCatalogueEntry;


/* EN: Collect all supported exports in compiled order while anchor is rooted.
 * Return zero for a missing, empty or overflowing catalogue. Borrow entries.
 * RU: Собрать поддерживаемые экспорты по порядку компиляции при удержанном anchor.
 * Вернуть ноль для отсутствующего, пустого или переполненного каталога. Записи заимствовать. */
static size_t loaded_catalogue(Modlink *anchor, LoadedCatalogueEntry *entries)
{
    Module *module = anchor_module(anchor);
    size_t count = 0;
    for (const Link *export = module ? module->ext : NULL; export && export->name; ++export)
    {
        StyxCallShape shape = {{0}, {0}};
        GatewayOwner gateway = {0};
        size_t length = styxscalar_name_length(export->name);
        if (!length || strchr(export->name, '/') || strchr(export->name, '\t') ||
            !export_profile(module, export, &shape, &gateway))
        {
            continue;
        }
        if (count == STYX_MOUNTED_EXPORT_LIMIT)
        {
            return 0;
        }
        entries[count++] = (LoadedCatalogueEntry){export, length, shape};
    }
    return count;
}


/* [GC:] EN: Copy the catalogue names from the rooted frame anchor to the result.
 * RU: Скопировать имена каталога из укоренённого anchor кадра в результат. */
void Vfsnative_scalarloadedexports(void *frame)
{
    F_Vfsnative_scalarloadedexports call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarloadedexports, ret), sizeof(*call.ret));
    LoadedCatalogueEntry entries[STYX_MOUNTED_EXPORT_LIMIT];
    size_t count = loaded_catalogue(dvm_sys_owner(&call.anchor), entries);
    char names[(INTERFACE_NAME_LIMIT + 1) * STYX_MOUNTED_EXPORT_LIMIT];
    size_t size = 0;
    for (size_t i = 0; i < count; ++i)
    {
        if (i)
        {
            names[size++] = '\t';
        }
        memcpy(names + size, entries[i].export->name, entries[i].name_length);
        size += entries[i].name_length;
    }
    if (size)
    {
        dvm_cap_publish_string(names, size, call.ret);
    }
    else
    {
        dvm_cap_publish(H, call.ret);
    }
}


/* [GC:] EN: Encode a complete private offer sequence from the rooted frame anchor.
 * Return an owned byte array or nil, without opening files or running guest methods.
 * RU: Закодировать полную последовательность частных предложений из anchor кадра.
 * Вернуть собственный массив или nil, без открытия файлов и выполнения методов гостя. */
void Vfsnative_scalarloadedprofile(void *frame)
{
    F_Vfsnative_scalarloadedprofile call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarloadedprofile, ret), sizeof(*call.ret));
    LoadedCatalogueEntry entries[STYX_MOUNTED_EXPORT_LIMIT];
    size_t count = loaded_catalogue(dvm_sys_owner(&call.anchor), entries);
    unsigned char profile[STYX_SHAPED_OFFER_MAX * STYX_MOUNTED_EXPORT_LIMIT];
    size_t size = 0;
    for (size_t i = 0; i < count; ++i)
    {
        const Link *export = entries[i].export;
        size_t used;
        if (!encode_offer(export->name, (uint32_t)export->sig, &entries[i].shape,
            profile + size, sizeof(profile) - size, &used))
        {
            dvm_cap_publish(H, call.ret);
            return;
        }
        size += used;
    }
    if (!size)
    {
        dvm_cap_publish(H, call.ret);
        return;
    }
    Array *result = H2D(Array *, heaparray(&Tbyte, (int)size));
    memcpy(result->data, profile, size);
    Setmark(D2H(result));
    Array *previous = dvm_sys_owner(call.ret);
    *call.ret = result;
    destroy(previous);
}
