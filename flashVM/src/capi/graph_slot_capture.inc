/* Copyright (c) 2026 bhgv
 * EN: Add sampled own slots to the existing graph identity and value encoding.
 * RU: Добавлять прочитанные слоты в существующее кодирование графа и его связей.
 */


/* [ASINC] [GC:] EN: Sample one native numeric binding. The reader must recognize
 * its backing accessor. Plain slots retain their exact original value.
 * RU: Прочитать числовую привязку через известный читателю внутренний аксессор.
 * Обычные слоты сохраняют исходное значение. */
FVM_Value project_index(FVM_Context *c, const FVM_Value &entry, const FVM_GraphSlotReader *reader)
{
    FVM_Object *holder = reader ? fvm_table_index_binding(entry) : NULL;
    if (!holder)
    {
        return entry;
    }
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::const_iterator accessor =
        holder->accessors.find(FVM_TABLE_INDEX_VALUE);
    require(accessor != holder->accessors.end(), FVM_UNSUPPORTED);
    FVM_Value sample;
    FVM_Result status = reader->read(c, holder, accessor->second, reader->state, &sample);
    require(status == FVM_OK, status == FVM_NOT_FOUND ? FVM_UNSUPPORTED : status);
    return sample;
}


/* [ASINC] [GC:] EN: Project an admitted own accessor as a value of dst. Native
 * table names remain strings. Array and standard Object indices restore as slots.
 * RU: Сохранить допущенный собственный аксессор как значение dst. Имена нативной
 * таблицы остаются строками. Индексы Array и Object восстанавливаются как слоты. */
bool project_property(FVM_Context *c, FVM_Object *owner, Object &dst, const fvm_detail::string &key,
    const FVM_AccessorEntry &entry, const FVM_GraphSlotReader *reader,
    fvm_detail::vector<FVM_Object *> &seen, size_t &budget)
{
    if (!reader)
    {
        return false;
    }
    FVM_Value sample;
    FVM_Result status = reader->read(c, owner, entry, reader->state, &sample);
    if (status == FVM_NOT_FOUND)
    {
        return false;
    }
    require(status == FVM_OK, status);
    unsigned long index;
    if ((owner->resource_kind == FVM_RESOURCE_ARRAY || owner->resource_kind == FVM_RESOURCE_OBJECT) &&
        fvm_property_terminated(key) && fvm_as_index(key.c_str(), &index))
    {
        if (dst.indexed.find(index) == dst.indexed.end())
        {
            dst.indexed[index] = capture_value(c, sample, seen, budget);
        }
    }
    else if (dst.properties.find(key) == dst.properties.end())
    {
        dst.properties[key] = capture_value(c, sample, seen, budget);
    }
    return true;
}
