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
    /* Tables are VM-owned objects. Numeric slots and named slots are distinct.
       Named properties support table-valued __index and __newindex. Numeric
       properties still use direct metatable fallback. Function-valued metamethods
       and metatables on primitive values are not implemented yet. */
    /* Update table index. */
    FVM_API FVM_Result fvm_table_set_index(FVM_Context *, FVM_Object *, unsigned long,
                                           const FVM_Value *);
    /* Read table index. */
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
    /* Implement table indexed at for this subsystem. */
    FVM_API FVM_Result fvm_table_indexed_at(FVM_Context *, FVM_Object *, size_t, unsigned long *,
                                            FVM_Value *);
#ifdef __cplusplus
}
#endif
#endif
