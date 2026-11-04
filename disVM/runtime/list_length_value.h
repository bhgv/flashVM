/* Pure native list-pointer snapshots and scalar length publication. */
#ifndef DVM_LIST_LENGTH_VALUE_H
#define DVM_LIST_LENGTH_VALUE_H
#include <stdint.h>


/* cell borrows one guarded pointer slot. Return captured borrowed pointer;
 * no referent access, allocation, owner/root or callback is performed. */
void *dvm_runtime_list_length_pointer(const void *cell);


/* count is prevalidated within native signed WORD maximum; destination is a
 * live guarded writable WORD. Return void after publication, no owner/callback. */
void dvm_runtime_list_length_value(uint64_t count, void *destination);
#endif
