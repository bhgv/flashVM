/* Copyright (c) 2026 bhgv
 * EN: Native file-set lifetime and exclusive per-open record staging, never VM heap pointers.
 * RU: Native lifetime file-set и отдельный staging записей каждого open, без указателей в VM heap.
 */
#ifndef STYX_ORDERED_FILES_INTERNAL_H
#define STYX_ORDERED_FILES_INTERNAL_H
#include "files.h"
#include "flashvm/srecord.h"
#include "cancel.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Caller retains owner until every exported factory/handle releases its pin.
 * Caller удерживает owner до освобождения pins всех exported factories/handles.
 */
struct StyxOrderedFiles
{
    pthread_mutex_t lock;      /* Protects exported factory count, never held across I/O/join. */
    size_t factories;          /* Factories include provider wrapper pins held by open handles. */
    StyxOrdered *peer;         /* Owned broker, disposed only by explicit final lifecycle free. */
};


/* EN/RU: One filtered provider; wrapper pins it through opened handle use.
 * Один фильтрованный provider; wrapper удерживает его во время работы open-handles.
 */
typedef struct OrderedFile
{
    StyxOrderedFiles *files;   /* Borrowed under this factory's owner pin. */
    StyxOrderedClient *client; /* Only this fixed granted tag. */
    unsigned long tag;         /* Application-local original S-record tag. */
    unsigned access;           /* Immutable local read/write ceiling. */
    size_t ceiling;            /* Immutable maximum local payload bytes. */
} OrderedFile;


/* EN/RU: FD owner serializes operations; one copied record survives short reads without cross-FD managed pointers.
 * FD-owner сериализует операции; копия одной записи переживает short reads без managed-указателей между FD.
 */
typedef struct OrderedFileOpen
{
    OrderedFile *file;         /* Borrowed through ordinary provider-wrapper pin. */
    unsigned access;           /* Exact opened read/write rights. */
    size_t offset;             /* Encoded prefix already copied from this staged record. */
    size_t extent;             /* Complete encoded staged record bytes. */
    unsigned char *payload;    /* Owned scratch for exactly one dequeued payload. */
    unsigned char *encoded;    /* Owned original S-record bytes, no outer framing. */
} OrderedFileOpen;


/* EN/RU: Borrow filtered factory; copy stable file metadata, no transport I/O.
 * Заимствовать filtered factory; скопировать стабильные metadata без transport I/O.
 */
FVM_Result orderedfileinfo(void *state, const char *path, FVM_FileInfo *output);


/* EN/RU: Opened handle callbacks borrow exclusive FD state; close consumes state, provider wrapper retains factory.
 * Callbacks заимствуют exclusive FD-state; close поглощает его, provider-wrapper удерживает factory.
 */
extern const DVM_VfsOps orderedfileops;
#endif
