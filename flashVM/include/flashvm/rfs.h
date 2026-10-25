/* Synthetic resource files: bounded native descriptors, no OS paths or implicit VM calls. */
#ifndef FLASHVM_RFS_H
#define FLASHVM_RFS_H                              /* Guard the portable C89 resource filesystem. */
#include "rwire.h"
#define FVM_RFS_FILES 16U                          /* Open native descriptors per authenticated connection. */
#define FVM_RFS_NAME FVM_R_PATH                    /* Maximum entry name bytes excluding NUL. */
#define FVM_RFS_BUFFER (FVM_RWIRE_BODY_MAX + 160U) /* Declaration plus one bounded S record. */
#define FVM_RFS_READ 1U                            /* Open for reading. */
#define FVM_RFS_WRITE 2U                           /* Open for writing; call files require READ|WRITE. */
#define FVM_RFS_ROOT 0U                            /* Root directory of visible resource incarnations. */
#define FVM_RFS_RESOURCE 1U                        /* One resource directory, named slot.generation. */
#define FVM_RFS_PATH 2U                            /* Original registration path as counted bytes. */
#define FVM_RFS_OPS 3U                             /* Directory of currently authorized operation IDs. */
#define FVM_RFS_EVENTS 4U                          /* Independent self-describing S-record event stream. */
#define FVM_RFS_CALL 5U                            /* One request and one self-describing S-record reply per open. */
#define FVM_RFS_QID_STRIDE 16UL                    /* Disjoint root/resource/child identity namespace. */
#define FVM_RFS_TAG 1UL                            /* File-local tag declared at the start of each binary output stream. */
#ifdef __cplusplus
extern "C"
{
#endif
/* Native identity; initialize all fields to zero for root. Never serialize this struct. */
typedef struct FVM_RFSNode
{
    FVM_RHandle target;      /* Exact resource incarnation; zero only at root. */
    unsigned int kind;       /* One of the RFS node classes. */
    unsigned long operation; /* Stable server operation ID for CALL; otherwise zero. */
} FVM_RFSNode;


/* Copied metadata, independent from descriptor and dispatcher storage. */
typedef struct FVM_RFSEntry
{
    FVM_RFSNode node;            /* Generation-checked native identity. */
    char name[FVM_RFS_NAME + 1]; /* Component name; root is slash. */
    int directory;               /* Nonzero for ROOT, RESOURCE and OPS. */
    unsigned int mode;           /* Supported native open bits, not an authority grant. */
    unsigned long qid_low;       /* Low Uint32 half of stable file identity. */
    unsigned long qid_high;      /* High Uint32 half: resource generation. */
} FVM_RFSEntry;


/* Descriptor incarnation, unrelated to the resource's own generation. */
typedef struct FVM_RFSFile
{
    unsigned long id;           /* One-based bounded file slot. */
    unsigned long generation;   /* Nonzero Uint32 generation; never wraps. */
} FVM_RFSFile;
typedef struct FVM_RFS FVM_RFS; /* Opaque fixed native storage; no managed roots. */


/* Borrow dispatcher until destroy; session must be authenticated and exclusive to this FS's
     * request IDs. Caller serializes every FS call. Dispatcher may separately use host mutex hooks.
     * Creation allocates fixed storage; outputs change only on OK. No rights are granted. */
FVM_API FVM_RResult fvm_rfs_create(FVM_RDispatch *, unsigned long session, FVM_RFS **);


/* Resolve current metadata; node names and authority are rechecked, output only on OK. */
FVM_API FVM_RResult fvm_rfs_stat(FVM_RFS *, FVM_RFSNode, FVM_RFSEntry *);


/* Walk exactly one component, including dot-dot; no ambient filesystem or slash aliases.
     * Root's parent is root. Outputs unchanged on errors. */
FVM_API FVM_RResult fvm_rfs_walk(FVM_RFS *, FVM_RFSNode, const char *name, FVM_RFSEntry *);


/* Open copies metadata; directories capture a discovery snapshot, events allocate one
     * independent watch, calls reserve no request until write. CALL requires READ|WRITE, others
     * READ only. */
FVM_API FVM_RResult fvm_rfs_open(FVM_RFS *, FVM_RFSNode, unsigned int mode, FVM_RFSFile *);


/* Directory index starts at zero; OK copies one entry, PENDING means end. Discovery changes
     * return CONFLICT until reopen, rather than silently skipping/revealing revoked snapshot
     * entries. */
FVM_API FVM_RResult fvm_rfs_readdir(FVM_RFS *, FVM_RFSFile, size_t index, FVM_RFSEntry *);


/* PATH permits arbitrary Uint32 offsets and EOF. CALL/EVENTS require the exact next stream
     * offset. PENDING means no reply/event yet, never EOF. Partial reads retain the encoded record;
     * current rights/watch incarnation are rechecked even for buffered bytes. Zero capacity is
     * INVALID. */
FVM_API FVM_RResult fvm_rfs_read(FVM_RFS *, FVM_RFSFile, unsigned long offset, void *,
                                 size_t capacity, size_t *used);


/* Exactly one atomic request payload (including empty) per CALL open, offset zero, <=R_BYTES.
     * OK means queued, never executed. Owner explicitly pumps the dispatcher. No console
     * dependency. Errors leave used and file state unchanged; accepted writes cannot be rolled
     * back. */
FVM_API FVM_RResult fvm_rfs_write(FVM_RFS *, FVM_RFSFile, unsigned long offset, const void *,
                                  size_t size, size_t *used);


/* Cancel queued request and acknowledge its reply, or discard a completed copy; running
     * callback BUSY leaves descriptor open. Close events discards only this reader. Stale
     * descriptors UNKNOWN. Never destroys registrations, disconnects sessions or rolls back
     * executed side effects. */
FVM_API FVM_RResult fvm_rfs_close(FVM_RFS *, FVM_RFSFile);


/* Quiesce all users first. Open descriptors return BUSY; successful destroy sets pointer NULL.
     */
FVM_API FVM_RResult fvm_rfs_destroy(FVM_RFS **);
#ifdef __cplusplus
}
#endif
#endif
