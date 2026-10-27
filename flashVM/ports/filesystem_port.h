/* Portable filesystem metadata and directory cursors; native APIs stay in OS ports. */
#ifndef FVM_FILESYSTEM_PORT_H
#define FVM_FILESYSTEM_PORT_H
#include "file_port.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define FVM_FS_NAME_MAX 4096U /* UTF-8 basename capacity including the terminator; no truncation. */
#define FVM_FS_BATCH_MAX 32U  /* Maximum directory entries returned in one owned native batch. */


/* Exact 64-bit bit pattern without requiring a C99 type in the embedding API. */
typedef struct FVM_FileNumber64
{
    unsigned long high; /* Most significant 32 bits, including a signed timestamp's sign. */
    unsigned long low;  /* Least significant 32 bits. */
} FVM_FileNumber64;


/* Copied native metadata. Timestamps are signed Unix seconds; identities are opaque. */
typedef struct FVM_FileInfo
{
    char name[FVM_FS_NAME_MAX]; /* Complete UTF-8 basename, independent of the cursor lifetime. */
    FVM_FileNumber64 length;    /* Nonnegative file length; directory lengths are native values. */
    FVM_FileNumber64 identity;  /* Native inode/file index; no VM pointer or pathname hash. */
    FVM_FileNumber64 device;    /* Native device/volume identity. */
    FVM_FileNumber64 accessed;  /* Signed access time encoded in two's-complement bits. */
    FVM_FileNumber64 modified;  /* Signed modification time encoded in two's-complement bits. */
    unsigned long mode;         /* Portable low nine permission bits, not native stat flags. */
    unsigned long uid;          /* Numeric owner identity when numeric_ids is true. */
    unsigned long gid;          /* Numeric group identity when numeric_ids is true. */
    int numeric_ids;            /* Zero where native numeric ownership is unavailable. */
    int directory;              /* Nonzero for a directory; symlink targets are followed. */
} FVM_FileInfo;

typedef struct FVM_PortDirectory FVM_PortDirectory; /* Owned, externally synchronized OS cursor. */


/* Query an open ordinary file, preserving both logical cursors. The captured
 * basename is a display name; identity/length come from the open object even
 * after rename/unlink. Borrow file/output; output changes only on success. */
FVM_Result fvm_port_file_info(FVM_PortFile *file, FVM_FileInfo *output);


/* Read path metadata, following links. Borrow UTF-8 path/output; publish output only
 * on success. Native I/O can block; errors do not fabricate records. */
FVM_Result fvm_port_path_info(const char *path, FVM_FileInfo *output);


/* Open a real directory cursor. Borrow path/output; transfer cursor on success only.
 * Android accepts ordinary filesystem paths, never content:// document-provider URIs. */
FVM_Result fvm_port_directory_open(const char *path, FVM_PortDirectory **output);


/* Read metadata of the opened directory, independent of later path lookup. */
FVM_Result fvm_port_directory_info(FVM_PortDirectory *directory, FVM_FileInfo *output);


/* Read at most FVM_FS_BATCH_MAX entries, omitting dot/dotdot. On success transfer
 * a batch (or NULL/count zero at EOF); release with entries_free. A failed read
 * invalidates the cursor to prevent silent skipping. No atomic snapshot is claimed. */
FVM_Result fvm_port_directory_read(FVM_PortDirectory *directory, FVM_FileInfo **entries, size_t *count);


/* Release a native batch, never a VM allocation; NULL is allowed. */
void fvm_port_directory_entries_free(FVM_FileInfo *entries);


/* Mark a cursor failed after caller-side publication/allocation failure. */
void fvm_port_directory_abort(FVM_PortDirectory *directory);


/* Close owned native cursor/resources; NULL is allowed. No VM owner is released here. */
void fvm_port_directory_close(FVM_PortDirectory *directory);
#ifdef __cplusplus
}
#endif
#endif
