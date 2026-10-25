/* Publish a real compiled module, pin its bytes, and release every native owner. */
#include "disvm/module_store.h" /* Versioned namespace and immutable reader API. */
#include "disvm/flashvm_host.h" /* Existing portable file-image acquisition. */
#include <errno.h>              /* Detect overflow while reading the byte count. */
#include <stdio.h>              /* Explain success and failure to the learner. */
#include <stdlib.h>             /* Parse the explicit framed module length. */


/* argv names an existing .dis file and its byte count; argc must equal three.
 * Returns zero after balanced ownership, one with a diagnostic on any failure.
 * This example stores module bytes; parsing/execution still require a VM owner. */
int main(int argc, char **argv)
{                                                                     /* Begin the native demonstration with no VM heap or scheduler. */
    DVM_ModuleStore *store = NULL;                                    /* Owner of namespace metadata and published image versions. */
    DVM_ModuleLease *lease = NULL;                                    /* Reader handle keeping one immutable version resident. */
    DVM_ModuleImage image = {0, 0, 0};                                /* Owned file image, separate from the namespace's copy. */
    DVM_ModuleView view;                                              /* Borrowed bytes, valid only while lease remains live. */
    unsigned long size;                                               /* Exact input length, supplied as framing metadata. */
    unsigned long generation = 0;                                     /* Publication ID written only after a successful commit. */
    char *end;                                                        /* First byte not consumed by numeric parsing. */
    int result = 1;                                                   /* Remain unsuccessful until every required operation succeeds. */
    if (argc != 3)                                                    /* Require both file identity and exact byte extent. */
    {                                                                 /* Report invocation syntax before allocating anything. */
        fprintf(stderr, "usage: module-store FILE.dis BYTE_COUNT\n"); /* Show the two required values. */
        return 1;                                                     /* Reject incomplete invocation. */
    } /* Finish argument-count validation. */
    errno = 0;                                                                                   /* Clear earlier library errors before strtoul. */
    size = strtoul(argv[2], &end, 10);                                                           /* Parse decimal metadata without evaluating shell text. */
    if (errno || !*argv[2] || *end || !size || size > DVM_MODULE_IMAGE_MAX || argv[2][0] == '-') /* Bound the image. */
    {                                                                                            /* Invalid framing must never start a file read. */
        fprintf(stderr, "invalid module byte count\n");                                          /* Explain the rejected extent. */
        return 1;                                                                                /* Preserve the filesystem and native ownership state. */
    } /* Finish extent validation. */
    store = dvm_module_store_create(4, DVM_MODULE_IMAGE_MAX); /* Allocate a four-name namespace. */
    if (!store)                                               /* Handle native allocation failure explicitly. */
    {                                                         /* No other owned resource exists yet. */
        fprintf(stderr, "cannot create module namespace\n");  /* Report the failed allocation. */
        return 1;                                             /* Stop before file acquisition. */
    } /* Continue only with a valid namespace owner. */
    if (dvm_image_file("/ide/main.dis", argv[1], (size_t)size, DVM_MODULE_IMAGE_MAX, &image) != DVM_IMAGE_OK) /* Read exact bytes. */
    {                                                                                                         /* The file adapter disposes partial reads on failure. */
        fprintf(stderr, "cannot acquire module image\n");                                                     /* Keep transport failure separate from parser errors. */
        goto cleanup;                                                                                         /* Release the namespace through the common owner path. */
    } /* The complete file image is now caller-owned. */
    if (dvm_module_store_publish(store, &image, 0, &generation) != DVM_STORE_OK) /* Create-only publication. */
    {                                                                            /* A failed commit preserves prior namespace state. */
        fprintf(stderr, "cannot publish module image\n");                        /* Report the namespace failure. */
        goto cleanup;                                                            /* Release the file image as well as the namespace. */
    } /* The namespace now owns its independent byte copy. */
    dvm_image_dispose(&image);                                                    /* Prove that published bytes do not borrow the compiler/file buffer. */
    if (dvm_module_store_acquire(store, "/ide/main.dis", &lease) != DVM_STORE_OK) /* Pin the published generation. */
    {                                                                             /* Lookup failure must not be mistaken for successful module execution. */
        fprintf(stderr, "cannot acquire published version\n");                    /* Describe the failed lookup. */
        goto cleanup;                                                             /* Keep cleanup centralized. */
    } /* The lease now protects its immutable byte extent. */
    if (dvm_module_lease_view(lease, &view) != DVM_STORE_OK) /* Borrow that extent without copying again. */
    {                                                        /* View construction validates its input handles. */
        fprintf(stderr, "cannot obtain module view\n");      /* Report the rejected view. */
        goto cleanup;                                        /* Release the lease before the namespace. */
    } /* Borrowed pointers remain usable until cleanup releases the lease. */
    printf("PASS stored %lu bytes at generation %lu; no VM execution\n", (unsigned long)view.size, generation); /* State the exact evidence. */
    result = 0;                                                                                                 /* All required operations completed. */
cleanup:                                                                                                        /* Both success and failure converge on balanced release. */
    dvm_module_lease_release(&lease);                                                                           /* Drop the reader before attempting store destruction. */
    dvm_image_dispose(&image);                                                                                  /* Safe after the earlier dispose, or after an acquisition failure. */
    if (dvm_module_store_destroy(&store) != DVM_STORE_OK)                                                       /* Do not ignore an unexpected retained reader. */
    {                                                                                                           /* Destruction refuses live leases instead of freeing their bytes. */
        fprintf(stderr, "module namespace is still busy\n");                                                    /* Make ownership failure visible. */
        result = 1;                                                                                             /* Cleanup failure also fails the demonstration. */
    } /* All owned resources are now released on the successful path. */
    return result; /* Return the observed outcome to the calling build/test script. */
} /* Finish the tutorial without constructing or pretending to run a Dis VM. */
