/* Copyright (c) 2026 bhgv
 * EN: Reusable Styx directory and per-fid callbacks for an integer array.
 * RU: Переиспользуемый каталог Styx и обработчики fid для массива целых.
 */
#ifndef STYX_INDEXED_FILES_H
#define STYX_INDEXED_FILES_H
#include "indexed_variable.h"

typedef struct StyxArrayFiles
{
    Styxserver *server; /* Borrowed server through fid retirement. */
    Path base; /* First qid of the array directory range. */
    StyxIntArray *array; /* Borrowed array through fid retirement. */
} StyxArrayFiles;

int styxarrayfilepublish(StyxArrayFiles *files, Styxserver *server, Path parent, Path base,
    const char *name, const char *owner, StyxIntArray *array);
char *styxarrayfileopen(StyxArrayFiles *files, Qid *qid, int mode);
char *styxarrayfileread(StyxArrayFiles *files, Qid qid, char *bytes, ulong *count, vlong offset);
char *styxarrayfilewrite(StyxArrayFiles *files, Qid qid, char *bytes, ulong *count, vlong offset);
void styxarrayfileclose(void *state);
#endif
