/* In-memory 9P2000 client/server tutorial: discover the original path through a synthetic file. */
#include "flashvm/r9p.h" /* Public C89 adapter; host authentication and transport are explicit. */
#include <stdio.h>       /* Print a human-readable result after validating binary replies. */
#include <string.h>      /* Compare counted bytes without assuming a NUL in a wire frame. */
/* A real service would inspect native owner state here; path reads do not invoke this callback. */
static FVM_RResult unused(const FVM_RRequest *q, void *out, size_t capacity, size_t *used, /* Unused owner callback still follows the bounded native handler ABI. */
                          void *host) /* Registration requires a handler. */
{                                     /* No managed references are retained. */
    (void)q; /* The metadata-only tutorial does not inspect a request. */
    (void)out; /* No result bytes are written by this placeholder handler. */
    (void)capacity; /* No output capacity is needed for an empty result. */
    (void)host;      /* This tutorial reads metadata only. */
    *used = 0;       /* Initialize the empty terminal result if called. */
    return FVM_R_OK; /* No implicit VM operation is performed. */
} /* Production operations use their own typed owner callbacks. */
/* Exercise actual independent standard wire fixtures; the public API is usable without main. */
int main(void) /* No socket, CLI parser, OS mount or VM is necessary. */
{              /* Every owned object starts null for common error cleanup. */
    static const unsigned char version[] = { /* Independent standard version negotiation fixture. */
        19, 0, 0, 0,   100, 255, 255, 0,   16, 0, /* Size, Tversion, NOTAG and first msize octets in little-endian order. */
        0,  6, 0, '9', 'P', '2', '0', '0', '0'}; /* Tversion: msize 4096. */
    static const unsigned char attach[] = { /* Independent root attach fixture for the trusted local session. */
        19, 0,   0,   0,   104, 1, 0, 0, 0, 0, /* Frame header, fid zero and the start of NOFID afid. */
        0,  255, 255, 255, 255, 0, 0, 0, 0}; /* Tattach fid zero; uname grants no rights. */
    static const unsigned char walk[] = { /* Independent walk fixture into the original-path file. */
        28, 0, 0, 0, 110, 2,   0,   0, 0, 0,   0,   1,   0,  0, 0, /* Header, old fid, new fid and the start of component count. */
        2,  0, 3, 0, '1', '.', '1', 4, 0, 'p', 'a', 't', 'h'}; /* Walk fresh 1.1/path into fid one.
                                                                */
    static const unsigned char open[] = {12, 0, 0, 0, 112, 3, /* Independent OREAD fixture for fid one. */
                                         0,  1, 0, 0, 0,   0}; /* Topen fid one, OREAD. */
    static const unsigned char read[] = { /* Independent counted read fixture with a split 64-bit offset. */
        23, 0, 0, 0, 116, 4, 0, 1, 0, 0, 0, 0, /* Header, fid one and initial zero offset octets. */
        0,  0, 0, 0, 0,   0, 0, 0, 2, 0, 0}; /* Tread offset zero, count 512. */
    const unsigned char *packets[] = {version, attach, walk, open, /* Keep packet pointers stable through each synchronous request. */
                                      read}; /* Borrow immutable test messages. */
    const size_t sizes[] = {sizeof(version), sizeof(attach), sizeof(walk), sizeof(open), /* Use exact fixture sizes rather than a guessed wire extent. */
                            sizeof(read)};                     /* Exact frame extents. */
    const unsigned char replies[] = {101, 105, 111, 113, 117}; /* Expected standard reply types. */
    FVM_RDispatch *dispatch = 0; /* Own the dispatcher until every adapter releases its borrow. */
    FVM_RFS *fs = 0;     /* Own one authenticated filesystem with an exclusive session namespace. */
    FVM_R9P *server = 0; /* Own one connection and all wire fids it opens. */
    FVM_ROperation operation = {1, FVM_R_READ}; /* Server-defined metadata fixture operation. */
    FVM_RHandle resource;                       /* Receive the exact resource generation. */
    unsigned char output[FVM_R9P_MSIZE];        /* Required complete-frame reply capacity. */
    size_t i, used = 0, consumed;               /* Keep produced and consumed lengths distinct. */
    int status = 1; /* Failure remains the default through partial setup. */
    if (fvm_rdispatch_create(0, &dispatch) != FVM_R_OK) /* Allocate single-owner native dispatch storage. */
        goto done; /* Single-owner native policy. */
    if (fvm_rdispatch_register(dispatch, "/scene/root", &operation, 1, unused, 0, &resource) != /* Register the host-owned resource metadata and callback. */
        FVM_R_OK) /* Check success before consuming the resource handle. */
        goto done; /* Copy registration. */
    if (fvm_rdispatch_grant(dispatch, resource, 1, FVM_R_READ) != FVM_R_OK) /* Native grants determine authority independently of attach uname. */
        goto done; /* Trusted local identity, not uname. */
    if (fvm_rfs_create(dispatch, 1, &fs) != FVM_R_OK) /* Acquire one FS using an exclusive trusted session. */
        goto done; /* Borrow dispatcher without starting I/O. */
    if (fvm_r9p_create(fs, &server) != FVM_R_OK) /* Acquire the server borrowing that filesystem. */
        goto done; /* Borrow FS and allocate bounded fid storage. */
    for (i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) /* Feed all five fixtures in reliable transport order. */
    { /* Feed one ordered frame at a time. */
        if (fvm_r9p_request(server, packets[i], sizes[i], output, sizeof(output), &used, /* Borrow the input only during this one-frame call. */
                            &consumed) != FVM_R_OK) /* Keep the produced reply and consumed byte counts separate. */
            goto done; /* Process one frame. */
        if (consumed != sizes[i] || used < 7 || output[4] != replies[i]) /* Validate frame consumption and the expected success type. */
            goto done; /* Reject incomplete or error replies. */
    } /* The final response is a counted ordinary path-file read. */
    if (used != 11 + sizeof("/scene/root") - 1) /* Require one whole counted path reply. */
        goto done; /* Require the exact reply extent. */
    if (memcmp(output + 11, "/scene/root", sizeof("/scene/root") - 1)) /* Compare payload bytes without expecting a trailing NUL. */
        goto done; /* Compare binary payload without NUL. */
    if (fvm_r9p_poll(server, output, sizeof(output), &used) != FVM_R_PENDING) /* There are no outstanding reads in this metadata-only exchange. */
        goto done;                         /* No unproduced reads remain. */
    puts("9p resource path: /scene/root"); /* Expected successful output. */
    status = 0; /* All protocol transactions and payload checks passed. */
done:           /* End connection ownership before ending its borrowed filesystem lifetime. */
    if (server && fvm_r9p_destroy(&server) != FVM_R_OK) /* End server ownership and close its wire fids first. */
        status = 1; /* Clunk owned fids; no running callbacks here. */
    if (fs && fvm_rfs_destroy(&fs) != FVM_R_OK) /* Release FS only after the server ends its borrow. */
        status = 1; /* All native descriptors are now closed. */
    if (dispatch && fvm_rdispatch_destroy(&dispatch) != FVM_R_OK) /* Release the dispatcher after all adapters are gone. */
        status = 1; /* Free registration storage last. */
    return status;  /* Presentation is independent from the library interface. */
} /* No worker, network listener or mount was created. */
