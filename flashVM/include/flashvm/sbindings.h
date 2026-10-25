/* Independent directional tag bindings for binary S-record channels; caller-owned bounded tables.
 */
#ifndef FLASHVM_SBINDINGS_H
#define FLASHVM_SBINDINGS_H        /* Guard the C89 binding registry ABI. */
#include "srecord.h"
#define FVM_SBINDINGS_VERSION 1UL  /* Version of the tag-zero payload schema. */
#define FVM_SBINDINGS_MAX 256U     /* Bound table size and duplicate-check work per declaration. */
#define FVM_SBINDINGS_TYPE_MAX 63U /* Printable ASCII identity bytes, excluding a trailing NUL. */
#ifdef __cplusplus
extern "C"
{
#endif
/* One immutable numeric tag to type identity/version association. */
typedef struct FVM_SBinding
{
    unsigned long tag;      /* Nonzero unsigned 32-bit local tag. */
    unsigned long version;  /* Nonzero unsigned 32-bit schema version. */
    unsigned int type_size; /* Identity bytes, 1..TYPE_MAX. */
    char type[FVM_SBINDINGS_TYPE_MAX +
              1];           /* Copied printable ASCII identity; lookup adds NUL. */
} FVM_SBinding;


/* One direction of one session. Keep TX and RX tables separate; do not modify fields directly.
     */
typedef struct FVM_SBindings
{
    size_t struct_size;  /* Exact descriptor size filled by init. */
    FVM_SBinding *slots; /* Caller-owned storage, stable until session disposal/reset. */
    size_t capacity;     /* Number of slots, at most MAX. */
    size_t count;        /* Occupied prefix; registry mutates only after complete validation. */
} FVM_SBindings;


/* Initialize an empty table without modifying slots; caller synchronizes all shared use.
     * Capacity zero/null slots is valid. Reinitializing invalidates prior session associations. */
FVM_API FVM_SResult fvm_sbindings_init(FVM_SBindings *, FVM_SBinding *, size_t capacity);


/* Copy an association to caller output. UNKNOWN for zero/unbound tags; outputs unchanged on
     * error. Lookup copies metadata, not a VM/native object; no allocation or implicit permission
     * grant. */
FVM_API FVM_SResult fvm_sbindings_lookup(const FVM_SBindings *, unsigned long tag,
                                         FVM_SBinding *);


/* Encode only the tag-zero payload, then wrap it with fvm_srecord_write(CONTROL_TAG,...).
     * Payload = schema version, count, repeated(tag, schema version, identity length, identity
     * bytes). Validates the full list/capacity before writing; no duplicate/zero tags, no NUL/space
     * identities. Input associations/output must not overlap; used remains unchanged on failure. */
FVM_API FVM_SResult fvm_sbindings_payload_write(const FVM_SBinding *, size_t count,
                                                void *output, size_t capacity, size_t *used);


/* Decode/checksum one complete tag-zero record and atomically append all new associations.
     * Exact repeats are idempotent; rebinding a tag returns CONFLICT, duplicates in one record
     * INVALID. Unknown payload version is INVALID. Input/outputs/table/slots must not overlap.
     * Failed/incomplete records leave every slot/count/used unchanged. No VM execution occurs. */
FVM_API FVM_SResult fvm_sbindings_apply(FVM_SBindings *, const void *, size_t size,
                                        size_t *used);
#ifdef __cplusplus
}
#endif
#endif
