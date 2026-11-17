/* Copyright (c) 2026 bhgv
 * EN: Exclusive per-open protocol buffers and one retained resource handle.
 * RU: Исключительные per-open protocol buffers и один удержанный resource handle.
 */
#ifndef STYX_MOVIE_SERVICE_INTERNAL_H
#define STYX_MOVIE_SERVICE_INTERNAL_H
#include "service.h"
#include "protocol.h"
#include "intent.h"
#include <stdlib.h>
#include <string.h>


typedef struct MovieOpen
{
    DVM_Namespace *group; /* Borrowed from provider pinned by the handle wrapper. */
    DVM_VfsHandle resource; /* Owned selected open until terminal record or close. */
    FVM_SBindings incoming; /* Direction-local request bindings. */
    FVM_SBinding slot; /* Storage for the dedicated binding. */
    unsigned char input[MOVIE_FRAME]; /* Owned incomplete request record. */
    size_t used; /* Input bytes retained across partial Twrites. */
    unsigned char output[MOVIE_FRAME]; /* Owned current response record. */
    size_t size; /* Complete current output record extent. */
    size_t sent; /* Prefix consumed by sequential Treads. */
    int stage; /* Zero idle, BEGIN/BYTES/END pending generation. */
    int failed; /* Sticky stream framing/protocol failure. */
    int requested; /* Exactly one acquisition per opened stream, then clunk. */
    int outcome; /* Acquisition failure, encoded only after staged byte prefix. */
    char origin[MOVIE_PATH]; /* Copied canonical namespace provenance. */
} MovieOpen;


/* EN/RU: Decode borrowed request into exclusive open;1/0, I/O may block, no VM entry.
 * Декодировать request в exclusive open;1/0, native blocking без VM entry. */
int movie_request(MovieOpen *opened, const unsigned char *bytes, size_t size);
#endif
