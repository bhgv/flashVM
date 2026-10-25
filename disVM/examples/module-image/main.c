/* Acquire actual module bytes from a file, then transfer the owned image through FlashVM. */
#include "disvm/flashvm_host.h" /* Shared file/channel image APIs and their ownership contract. */
#include <stdio.h>              /* Print native diagnostics; no Dis execution is implied. */
#include <stdlib.h>             /* Convert the resource's declared byte length. */
#include <errno.h>              /* Detect an out-of-range size argument. */
#include <string.h>             /* Compare complete binary module images, including NUL bytes. */


/* Run the host-transport demonstration; initialize no Dis heap or interpreter. */
int main(int argc, char **argv)           /* Arguments: an existing .dis path and its exact length. */
{                                         /* The caller owns both images and the channel until cleanup. */
    DVM_ModuleImage source = {0, 0, 0};   /* Native copy of the file image. */
    DVM_ModuleImage received = {0, 0, 0}; /* Independent copy received from the channel. */
    FVM_Channel *channel = 0;             /* Retained local buffered transport handle. */
    char *end = 0;                        /* First unparsed character after the length argument. */
    unsigned long size;                   /* Length supplied by resource metadata or the IDE. */
    int result = 1;                       /* Fail unless acquisition and binary comparison both succeed. */
    if (argc != 3)                        /* Require an explicit frame size; do not read a network stream until EOF. */
    {                                     /* Print the exact command shape for this native demonstration. */
        fprintf(stderr, "usage: module-image module.dis byte-count\n"); /* Explain the two inputs. */
        return 2;                                                       /* Report command-line misuse to the shell. */
    } /* Continue only with a complete request. */
    errno = 0;                                                 /* Reset conversion status before parsing the length. */
    size = strtoul(argv[2], &end, 10);                         /* Parse a decimal resource byte count. */
    if (errno || *end || !size || size > DVM_MODULE_IMAGE_MAX) /* Enforce native loader policy. */
    {
        return 2;                                              /* Reject invalid metadata before opening a file. */
    }
    if (dvm_image_file("example:input", argv[1], (size_t)size, DVM_MODULE_IMAGE_MAX, &source) !=
        DVM_IMAGE_OK)                           /* Copy through FlashVM's file port. */
    {
        goto cleanup;                           /* The API has already closed its private file handle on every path. */
    }
    if (fvm_channel_create(&channel) != FVM_OK) /* Use the real FlashVM channel implementation. */
    {
        goto cleanup;                           /* Preserve the acquired source until common cleanup. */
    }
    if (dvm_image_send_channel(channel, &source, 0) !=
        DVM_IMAGE_OK) /* Encode arbitrary bytes as a counted dis-hex message. */
    {
        goto cleanup; /* A failed send leaves source ownership here. */
    }
    if (dvm_image_channel("example:received", channel, 0, DVM_MODULE_IMAGE_MAX, &received) !=
        DVM_IMAGE_OK) /* Receive a separate native copy. */
    {
        goto cleanup; /* Invalid messages are consumed and released by the adapter. */
    }
    if (received.size != source.size ||
        memcmp(received.bytes, source.bytes, source.size)) /* Verify every byte rather than strlen. */
    {
        goto cleanup;                                      /* Binary inequality is a transport failure. */
    }
    printf("PASS acquired and transferred %lu module bytes; no VM execution\n",
           size);                     /* State the exact acceptance scope. */
    result = 0;                       /* Mark the native transport demonstration successful. */
cleanup:                              /* The cleanup order never depends on successful VM initialization. */
    if (channel)                      /* Creation may have failed before a channel existed. */
    {
        fvm_channel_release(channel); /* Release only this example's channel ownership. */
    }
    dvm_image_dispose(&received);     /* Free the received native byte/name allocation. */
    dvm_image_dispose(&source);       /* Free the source copy after the transport is done. */
    return result;                    /* Let the shell detect any failed acquisition or transfer. */
} /* No module entry point has been called and no interpreter has been started. */
