/* Checked scalar payload operations, reusable by native VM and host projections. */
#ifndef FVM_TABLE_REQUEST_H
#define FVM_TABLE_REQUEST_H
#include "table_owner.h"

typedef enum
{
    FVM_TABLE_GET,     /* Read one copied scalar. */
    FVM_TABLE_SET,     /* Insert/replace a copied scalar. */
    FVM_TABLE_DELETE,  /* Delete one key without compaction. */
    FVM_TABLE_LENGTH,  /* Read contiguous numeric prefix length. */
    FVM_TABLE_COUNT,   /* Count all key/value pairs. */
    FVM_TABLE_RESOLVE, /* Capture a canonical numeric key. */
    FVM_TABLE_RANGE,   /* Normalize signed bounds under the payload guard. */
    FVM_TABLE_SLICE,   /* Produce an independent copied prefix range. */
    FVM_TABLE_KEYS,    /* Produce an independent immutable key snapshot. */
    FVM_TABLE_SPLICE   /* Replace a range while guarding source and destination. */
} FvmTableOperation;

typedef struct
{
    FvmTableOperation operation; /* Selected operation, never interpreted as a VM opcode. */
    FhtKey key;                  /* Borrowed input key; RESOLVE replaces it on success. */
    int64_t value;               /* SET input, GET/LENGTH/COUNT output. */
    int64_t first;               /* Signed input; RANGE publishes absolute start. */
    int64_t end;                 /* Signed input; RANGE publishes exclusive end. */
    FvmTableOwner *replacement;  /* Borrowed live source, required only by SPLICE. */
    Fht *table;                  /* Owned SLICE result; caller retires/adopts on success. */
    FhtKeys *keys;               /* Owned KEYS result; caller retires on success. */
    FhtStatus status;            /* Application status, separate from native guard error. */
} FvmTableRequest;


/* Obtain a process-local, never-reused nonzero operation token. Borrowed result
 * changes only on success. Returns native error/EOVERFLOW; short metadata lock
 * only. Tokens must not be accepted from untrusted remote request fields. */
int fvm_owned_token(uint64_t *result);



/* Enter two stable guards in address order, collapsing identical owners.
 * Requires disabled pthread cancellation and the standard one-continuation token
 * contract. Returns native error; failure unwinds only entries acquired here.
 * Rejects pair-local reverse recursion with EDEADLK. The caller must also order
 * any unrelated guards it already holds; arbitrary lock upgrades are unsupported. */
int fvm_owned_enter_pair(FvmOwned *first, FvmOwned *second, uint64_t token);



/* Leave one matching pair entry in reverse order. Wrong token preserves both.
 * Inputs/token must match a successful pair entry; external lifetime applies. */
int fvm_owned_leave_pair(FvmOwned *first, FvmOwned *second, uint64_t token);



/* Execute one complete request on borrowed live owner(s), preserving token for
 * recursion. Requires disabled cancellation. Returns native error, while request
 * status reports Fht outcomes. Only scalar allocator callbacks run inside the
 * guards; these must return normally, without VM exceptions or nonlocal jumps.
 * All users, including readers, must use this boundary or the matching guard.
 * Direct destruction and raw payload access require external quiescence. */
int fvm_table_request(FvmTableOwner *owner, uint64_t token, FvmTableRequest *request);
#endif
