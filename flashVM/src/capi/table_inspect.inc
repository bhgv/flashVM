/* Copyright (c) 2026 bhgv
 * EN: C API enumeration for table entries and the context debugger.
 * RU: Перечислять записи таблиц и данные отладчика контекста в C API.
 */


/* EN: Count own named data entries.
 * RU: Подсчитать собственные именованные записи. */
FVM_Result fvm_table_named_count(FVM_Context *c, FVM_Object *o, size_t *out)
{
    if (!valid(c, o) || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = o->properties.size();
    return FVM_OK;
}


/* EN: Count own numeric entries, including bindings.
 * RU: Подсчитать числовые записи вместе с привязками. */
FVM_Result fvm_table_indexed_count(FVM_Context *c, FVM_Object *o, size_t *out)
{
    if (!valid(c, o) || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = o->indexed.size();
    return FVM_OK;
}


/* EN: Borrow own entry index and return its name and value.
 * RU: Вернуть заимствованные имя и значение записи index. */
FVM_Result fvm_table_named_at(FVM_Context *c, FVM_Object *o, size_t index, const char **name, FVM_Value *out)
{
    if (!valid(c, o) || !name || !out || index >= o->properties.size())
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_detail::map<fvm_detail::string, FVM_Value>::const_iterator it = o->properties.begin();
    while (index--)
    {
        ++it;
    }

    /* Computation / Вычисление и обновление состояния. */
    *name = it->first.c_str();
    *out = it->second;
    return FVM_OK;
}


/* EN: Return the byte count alongside the borrowed own name.
 * RU: Вернуть длину заимствованного собственного имени вместе с ним. */
FVM_Result
fvm_table_named_at_bytes(FVM_Context *c, FVM_Object *o, size_t index, const char **name, size_t *size, FVM_Value *out)
{
    if (!valid(c, o) || !name || !size || !out || index >= o->properties.size())
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_detail::map<fvm_detail::string, FVM_Value>::const_iterator entry = o->properties.begin();
    while (index--)
    {
        ++entry;
    }
    *name = entry->first.data();
    *size = entry->first.size();
    *out = entry->second;
    return FVM_OK;
}


/* EN: Read own numeric entry at index. A binding may wait for its value.
 * RU: Прочитать числовую запись по index. Чтение привязки может ждать ответа. */
FVM_Result fvm_table_indexed_at(FVM_Context *c, FVM_Object *o, size_t index, unsigned long *key, FVM_Value *out)
{
    if (!valid(c, o) || !key || !out || index >= o->indexed.size())
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_detail::map<unsigned long, FVM_Value>::const_iterator it = o->indexed.begin();
    while (index--)
    {
        ++it;
    }

    unsigned long found = it->first;
    FVM_Result status = fvm_table_read_entry(c, o, it->second, out);
    if (status == FVM_OK)
    {
        *key = found;
    }
    return status;
}


/* EN: Count all objects owned by c into out.
 * RU: Выдать в out число объектов контекста c. */
FVM_Result fvm_debug_object_count(FVM_Context *c, size_t *out)
{
    if (!c || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = c->objects.size();
    return FVM_OK;
}


/* EN: Borrow object i from c in out.
 * RU: Выдать в out заимствованный объект c по номеру i. */
FVM_Result fvm_debug_object_at(FVM_Context *c, size_t i, FVM_Object **out)
{
    if (!c || !out || i >= c->objects.size())
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = c->objects[i];
    return FVM_OK;
}


/* EN: Count native registry entries in c into out.
 * RU: Выдать число записей нативного реестра c в out. */
FVM_Result fvm_debug_native_function_count(FVM_Context *c, size_t *out)
{
    if (!c || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = c->functions.size();
    return FVM_OK;
}


/* EN: Borrow the native registry name at i in c.
 * RU: Выдать в out заимствованное имя c по номеру i. */
FVM_Result fvm_debug_native_function_at(FVM_Context *c, size_t i, const char **out)
{
    if (!c || !out || i >= c->functions.size())
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::const_iterator it = c->functions.begin();
    while (i--)
    {
        ++it;
    }
    *out = it->first.c_str();
    return FVM_OK;
}


/* EN: Count script registry entries in c into out.
 * RU: Выдать число записей скриптового реестра c в out. */
FVM_Result fvm_debug_script_function_count(FVM_Context *c, size_t *out)
{
    if (!c || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = c->script_functions.size();
    return FVM_OK;
}


/* EN: Borrow the script registry name at i in c.
 * RU: Выдать в out заимствованное имя c по номеру i. */
FVM_Result fvm_debug_script_function_at(FVM_Context *c, size_t i, const char **out)
{
    if (!c || !out || i >= c->script_functions.size())
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_detail::map<fvm_detail::string, FVM_ScriptFunction>::const_iterator it = c->script_functions.begin();
    while (i--)
    {
        ++it;
    }
    *out = it->first.c_str();
    return FVM_OK;
}


/* EN: Borrow the native registry key at index in c in out, and its byte count in size.
 * RU: Выдать в out имя нативной функции c по index и его длину в size, сохранив владение у c. */
FVM_Result fvm_debug_native_function_at_bytes(FVM_Context *c, size_t index, const char **out, size_t *size)
{
    if (!c || !out || !size || index >= c->functions.size())
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::const_iterator entry = c->functions.begin();
    while (index--)
    {
        ++entry;
    }
    *out = entry->first.data();
    *size = entry->first.size();
    return FVM_OK;
}


/* EN: Borrow the script registry key at index in c in out, and its byte count in size.
 * RU: Выдать в out имя скрипта c по index и его длину в size, сохранив владение у c. */
FVM_Result fvm_debug_script_function_at_bytes(FVM_Context *c, size_t index, const char **out, size_t *size)
{
    if (!c || !out || !size || index >= c->script_functions.size())
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_detail::map<fvm_detail::string, FVM_ScriptFunction>::const_iterator entry = c->script_functions.begin();
    while (index--)
    {
        ++entry;
    }
    *out = entry->first.data();
    *size = entry->first.size();
    return FVM_OK;
}
