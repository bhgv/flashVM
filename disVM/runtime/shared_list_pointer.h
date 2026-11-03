/* Rooted native pointer-list projection, independent of serialized file ABI. */
#ifndef DVM_SHARED_LIST_POINTER_H
#define DVM_SHARED_LIST_POINTER_H


/* [GC:] source is an independently owned mapped list-reference cell; destination
 * is a live writable pointer cell. tail must be 0 (head pointer) or 1 (list tail).
 * Under ACTIVE worker admission, retain/root the list before interior access,
 * validate allocated extent/schema, use guarded MOVP, retire displaced output
 * outside guards and finish the list hold on success/error. Return void normally;
 * errors propagate after native cleanup. No arbitrary-pointer/GC epoch or parallel
 * managed admission capability follows. Caller owns cell/module lifetimes. */
void dvm_runtime_list_pointer_copy(const void *source, void *destination, int tail);
#endif
