/* Guarded LENL traversal; continuous source and discovered-link exclusion. */
#ifndef DVM_SHARED_LIST_LENGTH_H
#define DVM_SHARED_LIST_LENGTH_H


/* [GC:] source/destination borrow live reference/WORD cells with caller-proven
 * container/referent/root lifetime. Return static error or NULL after unlock.
 * Keep source/visited links guarded until result publication, with no root,
 * retain, allocation, safepoint or VM callback. Nil/H distinctions and cycles
 * match retained LENL. No parallel collector/admission permission is supplied. */
const char *dvm_runtime_list_length(const void *source, void *destination);
#endif
