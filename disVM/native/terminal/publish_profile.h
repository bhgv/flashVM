/* Copyright (c) 2026 bhgv
 * EN: Admit private Dis offers before exposing the mounted Styx interface.
 * RU: Проверить частные предложения Dis перед публикацией интерфейса Styx.
 */
#ifndef STYX_PUBLISH_PROFILE_H
#define STYX_PUBLISH_PROFILE_H

#include "load_profile.h"
#include "scalar_verify.h"

typedef struct StyxPublishedExport
{
    char name[INTERFACE_NAME_LIMIT + 1]; /* Owned exact compiled export name. */
    StyxCallShape shape;                /* Copied schema, empty for a legacy offer. */
    unsigned long request;              /* Request tag selected by the verified offer. */
    int scoped;                         /* Route requires the session's reverse root. */
} StyxPublishedExport;


/* EN: Admit a complete borrowed offer sequence and encode the public profile in order.
 * If name is supplied require a single offer for that exact name. scoped enables the
 * reverse protocol for shape-driven methods. On failure leave all outputs unchanged.
 * RU: Проверить полную последовательность и закодировать открытый профиль по порядку.
 * При заданном name требуется одна запись с этим именем. scoped включает встречный
 * протокол для методов со схемой. При отказе все выходные значения сохраняются. */
int styxpublishprofile(const void *offers, size_t size, const char *name, int scoped,
    void *profile, size_t capacity, size_t *used, StyxPublishedExport *exports, size_t limit, size_t *count);

#endif
