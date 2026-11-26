/* Copyright (c) 2026 bhgv
 * EN: Private, traced backing object for a live native numeric table entry.
 * RU: Внутренний отслеживаемый объект живой числовой записи нативной таблицы.
 */
#ifndef FVM_TABLE_INDEX_BINDING_HPP
#define FVM_TABLE_INDEX_BINDING_HPP
#include "../core/object_root_scope.hpp"

#define FVM_TABLE_INDEX_VALUE "value" /* Private accessor on the backing object. */


/* [GC:] EN: Recognize the private backing object stored in a numeric entry.
 * RU: Распознать внутренний объект, сохранённый в числовой записи. */
inline FVM_Object *fvm_table_index_binding(const FVM_Value &entry)
{
    return entry.type == FVM_VALUE_OBJECT && entry.object &&
        entry.object->resource_kind == FVM_RESOURCE_TABLE_INDEX_BINDING ? entry.object : NULL;
}

/* [ASINC] [GC:] EN: Return the stored value or read its live binding. Keep the receiver
 * reachable through a getter. The getter may block on the owning RPC session.
 * RU: Вернуть значение либо прочитать живую привязку, удерживая владельца
 * через вызов getter. Getter может ждать ответа сессии RPC. */
inline FVM_Result fvm_table_read_entry(FVM_Context *c, FVM_Object *receiver, const FVM_Value &entry, FVM_Value *out)
{
    FVM_Object *holder = fvm_table_index_binding(entry);
    if (holder)
    {
        FVM_ObjectRootScope keep_receiver(c, receiver);
        return fvm_object_get(c, holder, FVM_TABLE_INDEX_VALUE, out);
    }
    *out = entry;
    return FVM_OK;
}


#endif
