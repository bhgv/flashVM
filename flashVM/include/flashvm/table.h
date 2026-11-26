/* FlashVM: Public embedding API declarations: table.
 * File: include/flashvm/table.h.
 */

#ifndef FLASHVM_TABLE_H
/* Define FLASHVM TABLE H constant or compilation switch. */
#define FLASHVM_TABLE_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Legacy native tables have distinct integer and named slots. Standard Object,
       Array and arguments route canonical indexed properties through their shared
       property bridge. Other standard/global AVM1 member names use string keys.
       Named __index/__newindex support table and function hooks; raw native integer
       slots retain direct table-metatable fallback. Primitive metadata uses value APIs. */


/* EN: Store value at key on object. A private reference binds a free native slot,
 * and later writes update its target. A live remote write can block or fail.
 * RU: Записать значение по key в object. Внутренняя ссылка привязывает свободный
 * числовой слот. Последующие записи идут в источник и могут ждать ответа или отказать. */
FVM_API FVM_Result fvm_table_set_index(FVM_Context *, FVM_Object *, unsigned long,
                                       const FVM_Value *);


/* EN: Return the current numeric entry at key. A live binding can wait for its peer.
 * RU: Прочитать текущую числовую запись по key. Живая привязка может ждать ответа участника. */
FVM_API FVM_Result fvm_table_get_index(FVM_Context *, FVM_Object *, unsigned long, FVM_Value *);


/* Update table metatable. */
FVM_API FVM_Result fvm_table_set_metatable(FVM_Context *, FVM_Object *, FVM_Object *);


/* Read table metatable. */
FVM_API FVM_Result fvm_table_get_metatable(FVM_Context *, FVM_Object *, FVM_Object **);


/* Count table named. */
FVM_API FVM_Result fvm_table_named_count(FVM_Context *, FVM_Object *, size_t *);


/* Count table indexed. */
FVM_API FVM_Result fvm_table_indexed_count(FVM_Context *, FVM_Object *, size_t *);


/* Implement table named at for this subsystem. */
FVM_API FVM_Result fvm_table_named_at(FVM_Context *, FVM_Object *, size_t, const char **,
                                      FVM_Value *);


/* Return a borrowed counted name and value; pointers expire on mutation. */
FVM_API FVM_Result fvm_table_named_at_bytes(FVM_Context *, FVM_Object *, size_t,
                                            const char **, size_t *, FVM_Value *);


/* EN: Return the key and current value of an own numeric entry by ordinal position.
 * A live value is resolved before returning the key.
 * RU: Прочитать ключ и текущее значение числовой записи по порядковому номеру.
 * Живая привязка разрешается до выдачи ключа. */
FVM_API FVM_Result fvm_table_indexed_at(FVM_Context *, FVM_Object *, size_t, unsigned long *,
                                        FVM_Value *);
#ifdef __cplusplus
}
#endif
#endif
