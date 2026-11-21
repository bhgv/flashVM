/* Copyright (c) 2026 bhgv
 * EN: Bind a typed hybrid-table key to a selector fid and its scoped value fids.
 * RU: Связать типизированный ключ таблицы с fid выбора и файлами значения.
 */

/* EN: Decode one complete typed key from borrowed bytes.
 * RU: Декодировать полный типизированный ключ из заимствованных байтов. */
static int decode_key(const unsigned char *bytes, size_t size, FhtKey *key)
{
    if (!size)
    {
        return 0;
    }
    if (!bytes[0] && size == 9)
    {
        *key = (FhtKey){.kind = FHT_INDEX, .index = word(bytes + 1)};
        return 1;
    }
    if (bytes[0] == 1 && size <= TABLE_KEY_LIMIT + 1)
    {
        *key = (FhtKey){.kind = FHT_TEXT, .text = (const char *)bytes + 1, .size = size - 1};
        return 1;
    }
    return 0;
}


/* EN: Copy the canonical key into the selector; subsequent readers retain it.
 * RU: Скопировать канонический ключ в выбор, который удерживают читатели. */
static void publish_selection(TableCursor *cursor, FhtKey key)
{
    TableProjection *table = cursor->node->table;
    TableSelection *selection = cursor->selection;
    fvm_port_sync_lock(table->lock);
    if (key.kind == FHT_TEXT)
    {
        memcpy(selection->text, key.text, key.size);
        key.text = selection->text;
    }
    selection->key = key;
    selection->selected = 1;
    fvm_port_sync_unlock(table->lock);
}


/* EN: Test revocation before operating on a retained value fid.
 * RU: Проверить отзыв перед операцией с удерживаемым fid значения. */
static int selection_lives(const TableCursor *cursor)
{
    return !cursor->selection || __atomic_load_n(&cursor->selection->alive, __ATOMIC_ACQUIRE);
}


/* EN: Adopt one selector per caller or pin a fully populated key for a value fid.
 * RU: Принять один выбор на клиента или удержать готовый ключ для fid значения. */
static int open_selection(TableCursor *cursor)
{
    DVM_ProviderOpenScope scope = dvm_collection_open_scope();
    if (!scope.domain)
    {
        return 0;
    }
    int selector = cursor->node->kind == Index;
    TableSelection *created = selector ? calloc(1, sizeof(*created)) : NULL;
    if (selector && !created)
    {
        return 0;
    }
    TableProjection *table = cursor->node->table;
    fvm_port_sync_lock(table->lock);
    TableSelection *active = table->selectors;
    while (active && (active->scope.domain != scope.domain || active->scope.generation != scope.generation))
    {
        active = active->next;
    }
    if (created && !active)
    {
        created->next = table->selectors;
        created->scope = scope;
        created->references = 1;
        created->alive = 1;
        table->selectors = cursor->selection = created;
        created = NULL;
    }
    else if (!selector && active && active->selected && active->references < SIZE_MAX)
    {
        active->references++;
        cursor->selection = active;
    }
    fvm_port_sync_unlock(table->lock);
    free(created);
    return cursor->selection != NULL;
}


/* EN: Unlink and revoke a selector, or just release a value fid's pin.
 * RU: Снять и отозвать выбор либо освободить ссылку fid значения. */
static void close_table_cursor(void *state)
{
    TableCursor *cursor = state;
    TableSelection *selection = cursor->selection;
    if (selection)
    {
        TableProjection *table = cursor->node->table;
        fvm_port_sync_lock(table->lock);
        if (cursor->node->kind == Index)
        {
            TableSelection **link = &table->selectors;
            while (*link && *link != selection)
            {
                link = &(*link)->next;
            }
            if (*link)
            {
                *link = selection->next;
            }
            __atomic_store_n(&selection->alive, 0, __ATOMIC_RELEASE);
        }
        int last = --selection->references == 0;
        fvm_port_sync_unlock(table->lock);
        if (last)
        {
            free(selection);
        }
    }
    free(cursor);
}
