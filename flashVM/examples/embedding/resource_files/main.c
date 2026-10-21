/* Portable resource files: queue a call, pump on the owner, and read its binary reply. */
#include "flashvm/rfs.h"       /* Native C89 file API; no OS mount is required. */
#include <stdio.h>             /* Print the validated tutorial result. */
#include <string.h>            /* Initialize native descriptors and copy counted payloads. */
#define TUTORIAL_SESSION 1UL   /* Trusted local session, exclusively owned by this filesystem. */
#define TUTORIAL_OPERATION 7UL /* Server-defined read operation; unrelated to S-record tags. */
/* Echo counted bytes without retaining the request or touching managed VM objects. */
static FVM_RResult echo(const FVM_RRequest *request, void *out, size_t capacity, size_t *used, /* Owner callback receives a counted request and a bounded reply buffer. */
                        void *host) /* Owner callback. */
{               /* The dispatcher invokes this only during explicit owner pumping. */
    (void)host; /* No host state is needed for this native example. */
    if (request->size > capacity) /* Validate the reply extent before copying. */
        return FVM_R_LIMIT; /* Reject insufficient output storage. */
    if (request->size) /* An empty payload requires no memory access. */
        memcpy(out, request->data, request->size); /* Copy the borrowed request now. */
    *used = request->size;                         /* Publish the initialized reply extent. */
    return FVM_R_OK; /* The result is terminal, not an asynchronous promise. */
} /* No pointer escapes the handler. */
/* Run one complete file lifecycle; errors still close files before destroying their dispatcher. */
int main(void) /* Standard console wrapper; all resource operations are callable directly from C. */
{              /* Native ownership starts here. */
    FVM_RDispatch *dispatch = 0; /* Own one dispatcher through final cleanup. */
    FVM_RFS *fs = 0;             /* Own one authenticated filesystem borrowing that dispatcher. */
    FVM_ROperation operation = {TUTORIAL_OPERATION, FVM_R_READ}; /* Immutable operation policy. */
    FVM_RHandle resource;                 /* Receive the registered resource incarnation. */
    FVM_RFSNode root;                     /* Zero is the canonical synthetic root identity. */
    FVM_RFSEntry object, directory, call; /* Copied entries contain no managed references. */
    FVM_RFSFile file;                     /* Track the only open descriptor for cleanup. */
    FVM_SBinding slot;                    /* Own the incoming file-local binding storage. */
    FVM_SBindings bindings; /* Decode the tag-zero declaration before its typed reply. */
    FVM_RReply reply;       /* Own a copied terminal result. */
    unsigned char bytes[FVM_RFS_BUFFER];        /* Retain one bounded self-describing response. */
    unsigned long request;                      /* Receive the server-assigned request ID. */
    size_t used, total, completed, declaration; /* Keep I/O and owner execution counts distinct. */
    int opened = 0, result = 1;     /* Failure is the default; cleanup must preserve open state. */
    memset(&root, 0, sizeof(root)); /* Initialize every root identity field. */
    if (fvm_rdispatch_create(0, &dispatch) != FVM_R_OK) /* Create native single-owner queue storage. */
        goto done; /* Single-owner policy. */
    if (fvm_rdispatch_register(dispatch, "/scene/root", &operation, 1, echo, 0, &resource) != /* Register the copied name and immutable operation policy. */
        FVM_R_OK) /* Check registration before using the output handle. */
        goto done; /* Copy registration. */
    if (fvm_rdispatch_grant(dispatch, resource, TUTORIAL_SESSION, FVM_R_READ) != FVM_R_OK) /* Grant authority only to the trusted tutorial session. */
        goto done; /* Explicit authority. */
    if (fvm_rfs_create(dispatch, TUTORIAL_SESSION, &fs) != FVM_R_OK) /* Create the FS borrowing the live dispatcher. */
        goto done; /* Borrow the dispatcher. */
    if (fvm_rfs_walk(fs, root, "1.1", &object) != FVM_R_OK) /* Resolve this fresh resource incarnation. */
        goto done; /* Fresh dispatcher uses slot 1, generation 1. */
    if (fvm_rfs_walk(fs, object.node, "ops", &directory) != FVM_R_OK) /* Walk to the authorized operation directory. */
        goto done; /* Resolve authorized operations. */
    if (fvm_rfs_walk(fs, directory.node, "7", &call) != FVM_R_OK) /* Resolve the numeric server operation. */
        goto done; /* Resolve the server operation ID. */
    if (fvm_rfs_open(fs, call.node, FVM_RFS_READ | FVM_RFS_WRITE, &file) != FVM_R_OK) /* Acquire a one-call descriptor with both supported I/O bits. */
        goto done; /* One call per open. */
    opened = 1;    /* Cleanup now owns an open descriptor. */
    if (fvm_rfs_write(fs, file, 0, "hello", 5, &used) != FVM_R_OK) /* Post copied parameters without executing the owner callback. */
        goto done; /* Queue copied bytes; do not execute yet. */
    if (fvm_rdispatch_pump(dispatch, 1, &completed) != FVM_R_OK || completed != 1) /* Execute exactly the expected queued job on its owner. */
        goto done; /* Execute on the owner. */
    total = 0;     /* Begin the binary response stream at offset zero. */
    do /* Repeat until the completed response reaches EOF. */
    { /* Read small pieces to demonstrate retention across partial reads. */
        if (total == sizeof(bytes)) /* Leave room before each single-byte read. */
            goto done; /* Keep every output write within owned storage. */
        if (fvm_rfs_read(fs, file, (unsigned long)total, bytes + total, 1, &used) != FVM_R_OK) /* Read the next exact stream byte into owned storage. */
            goto done; /* Exact next offset. */
        total += used; /* Advance only after successful copying. */
    } while (used); /* A completed one-call file ends with a zero-byte read. */
    if (fvm_sbindings_init(&bindings, &slot, 1) != FVM_S_OK) /* Initialize one file-local incoming binding slot. */
        goto done; /* Start a fresh file-local RX table. */
    if (fvm_sbindings_apply(&bindings, bytes, total, &declaration) != FVM_S_OK) /* Apply the checksummed declaration before its typed data. */
        goto done; /* Verify the declaration CRC. */
    if (fvm_rreply_record_read(&bindings, bytes + declaration, total - declaration, &request, /* Decode a complete typed reply from the remaining stream. */
                               &reply, &used) != FVM_S_OK) /* Publish copied reply fields only after validation succeeds. */
        goto done; /* Verify reply schema. */
    if (reply.result != FVM_R_OK || reply.size != 5 || memcmp(reply.data, "hello", 5)) /* Validate the terminal result and counted echo bytes. */
        goto done;                 /* Check counted content. */
    puts("resource files: hello"); /* Expected successful output. */
    result = 0;                    /* The complete request/reply contract passed. */
done:                              /* Cleanup also handles partially initialized setup. */
    if (opened && fvm_rfs_close(fs, file) != FVM_R_OK) /* Close only if setup actually acquired a descriptor. */
        result = 1; /* Cancel queued work or release a consumed reply. */
    if (fs && fvm_rfs_destroy(&fs) != FVM_R_OK) /* End the dispatcher borrow after all files close. */
        result = 1; /* Release native storage before its borrow ends. */
    if (dispatch && fvm_rdispatch_destroy(&dispatch) != FVM_R_OK) /* Release the dispatcher after its filesystem is gone. */
        result = 1; /* Registration userdata is not owned here. */
    return result;  /* Report normal C process success or failure. */
} /* No OS descriptor, socket, worker or mount was created. */
