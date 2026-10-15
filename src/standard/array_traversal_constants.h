/* FlashVM: retained bounds for host mutation and sorting. */
#ifndef FVM_ARRAY_TRAVERSAL_CONSTANTS_H
#define FVM_ARRAY_TRAVERSAL_CONSTANTS_H /* Header guard. */
/* Host mutation paths retain their existing traversal restriction. */
#define FVM_ARRAY_HOST_MUTATION_MAX_LENGTH 65536UL
/* Host generic sort snapshots retain their independent bounded representation. */
#define FVM_ARRAY_HOST_GENERIC_SORT_MAX_LENGTH 65536UL
/* Host native sort retains its original allocation-bound rejection policy. */
#define FVM_ARRAY_HOST_NATIVE_SORT_MAX_LENGTH 65536UL
#endif
