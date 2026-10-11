#ifndef FLASHVM_TABLE_H
#define FLASHVM_TABLE_H
#include "value.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Tables are VM-owned objects. Numeric slots and named slots are distinct.
   Metatable fallback is used only when an own slot is absent; it is not yet
   the full Lua __index/__newindex metamethod protocol. */
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
