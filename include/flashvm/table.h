#ifndef FLASHVM_TABLE_H
#define FLASHVM_TABLE_H
#include "value.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Tables are VM-owned objects. Numeric slots and named slots are distinct.
   Named properties support table-valued __index and __newindex. Numeric
   properties still use direct metatable fallback. Function-valued metamethods
   and metatables on primitive values are not implemented yet. */
FVM_API FVM_Result fvm_table_set_index(FVM_Context *, FVM_Object *, unsigned long, const FVM_Value *);
FVM_API FVM_Result fvm_table_get_index(FVM_Context *, FVM_Object *, unsigned long, FVM_Value *);
FVM_API FVM_Result fvm_table_set_metatable(FVM_Context *, FVM_Object *, FVM_Object *);
FVM_API FVM_Result fvm_table_get_metatable(FVM_Context *, FVM_Object *, FVM_Object **);
FVM_API FVM_Result fvm_table_named_count(FVM_Context *, FVM_Object *, size_t *);
FVM_API FVM_Result fvm_table_indexed_count(FVM_Context *, FVM_Object *, size_t *);
FVM_API FVM_Result fvm_table_named_at(FVM_Context *, FVM_Object *, size_t, const char **, FVM_Value *);
FVM_API FVM_Result fvm_table_indexed_at(FVM_Context *, FVM_Object *, size_t, unsigned long *, FVM_Value *);
#ifdef __cplusplus
}
#endif
#endif
