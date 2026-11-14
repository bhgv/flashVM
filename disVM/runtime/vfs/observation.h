/* Copyright (c) 2026 bhgv
 * EN: On-demand bounded namespace text observations, without host I/O or new catalogues.
 * RU: Ограниченные namespace observations по запросу, без host I/O и новых каталогов.
 */
#ifndef DVM_VFS_OBSERVATION_H
#define DVM_VFS_OBSERVATION_H
#include "namespace.h"
#include <stddef.h>

#define DVM_NS_OBSERVE_BYTES 128     /* Maximum copied text bytes per debugger response. */
#define DVM_NS_FIELD_ROOT 0          /* Captured native root display spelling. */
#define DVM_NS_FIELD_CWD 1           /* Virtual current directory. */
#define DVM_NS_FIELD_IMPORT 2        /* Virtual import directory. */
#define DVM_NS_FIELD_CWD_SOURCE 3    /* Captured display provenance of held cwd. */
#define DVM_NS_FIELD_IMPORT_SOURCE 4 /* Captured display provenance of import base. */
#define DVM_NS_FIELD_TARGET 5        /* Binding index selects virtual target. */
#define DVM_NS_FIELD_SOURCE 6        /* Binding index selects captured native source. */
#define DVM_NS_FIELD_COMPONENT 7     /* Binding index selects file basename or empty directory suffix. */
#define DVM_NS_OBSERVED 1            /* Successful coherent copy. */
#define DVM_NS_OBSERVE_STALE 0       /* Expected revision differs; no text copied. */
#define DVM_NS_OBSERVE_INVALID -1    /* Invalid index/field/offset or missing storage. */


/* EN/RU: Native copied values only; counts describe one lock instant, not a heap census.
 * Только copied native values; counts относятся к одному моменту, не heap census. */
typedef struct DVM_NamespaceObservation
{
    uint64_t version;  /* Mutation revision; compare across text requests. */
    size_t bindings;   /* Current number of replacement records. */
    size_t references; /* Group owners at this instant, including startup controller. */
    size_t length;     /* Full byte length of selected text, zero for summary. */
    size_t copied;     /* Bytes copied to caller, without a NUL terminator. */
    int root_held;     /* Root has a retained directory capability. */
    int cwd_held;      /* Current directory has a retained capability. */
    int import_held;   /* Import directory has a retained capability. */
    int binding_held;  /* Selected binding owns a directory capability. */
} DVM_NamespaceObservation;


/* EN: Read summary when expected is NULL; otherwise copy at most capacity bytes
 * from field/index/offset only at *expected revision. Caller holds group lifetime,
 * output and bytes storage. No allocation/I/O/VM calls. Group mutex protects copy.
 * Return OBSERVED/STALE/INVALID; output summary is set on stale, bytes untouched.
 * Index is zero for group fields; binding order is stable only at same revision.
 * RU: При expected=NULL читать summary, иначе копировать не более capacity bytes
 * field/index/offset только при версии *expected. Caller удерживает group и outputs.
 * Без allocation/I/O/VM. Copy под group mutex. OBSERVED/STALE/INVALID; при stale
 * summary задан, bytes не меняются. Group fields требуют index=0; binding order
 * стабилен только при той же revision. Offset=length допустим и даёт copied=0. */
int dvm_namespace_observe(DVM_Namespace *group,
                          const uint64_t *expected,
                          size_t index,
                          int field,
                          size_t offset,
                          unsigned char *bytes,
                          size_t capacity,
                          DVM_NamespaceObservation *output);
#endif
