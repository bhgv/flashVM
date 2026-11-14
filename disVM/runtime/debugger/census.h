/* Copyright (c) 2026 bhgv
 * EN: Temporary bounded GC-map census of one retained continuation's reachable graph.
 * RU: Временный bounded GC-map census reachable graph одного retained continuation.
 */
#ifndef DVM_DEBUG_CENSUS_H
#define DVM_DEBUG_CENSUS_H
#include "disvm/range_access.h"

#define CENSUS_NODES 256      /* Native scratch exists only during an explicit query. */
#define CENSUS_FIELDS 384     /* Includes root slots and traversed managed edges. */
#define CENSUS_MAP_BYTES 4096 /* Bound scanning even maps containing no references. */


/* EN/RU: Query-owned scratch; guards keep incoming strong edges live until release.
 * Query-owned scratch; guards удерживают incoming strong edges до release. */
typedef struct DebugCensus
{
    DVM_RangeAccess access;      /* One shared guard set; never stored after the query. */
    void *nodes[CENSUS_NODES];   /* Borrowed payloads under guarded incoming edges. */
    unsigned int index[512];     /* Deduplicate cycles/aliases, index+1 in open hash. */
    unsigned int count;          /* Unique currently guarded objects. */
    unsigned int fields;         /* Total guarded root/edge cells processed. */
    unsigned int maps;           /* Descriptor bytes visited, including zero maps. */
    unsigned int roots;          /* Nonempty strong root occurrences, including aliases. */
    unsigned int frames;         /* All allocated GC stack frames, including prepared/native frames. */
    unsigned long frame_bytes;   /* Frame body extents, excluding unused stack capacity. */
    unsigned long bytes;         /* Sum pool capacity including Heap headers, not requested payload/RSS. */
    unsigned int kinds[7];       /* Array/string/list/channel/modlink/record/opaque objects. */
    unsigned long kind_bytes[7]; /* Capacity bytes in the corresponding categories. */
    unsigned int flags;          /* 1 complete;2 limit;4 contention;8 invalid;16 opaque GC callback. */
} DebugCensus;


/* [GC:] EN/RU: Add guarded strong root/edge; no retain/mark/destructor or arbitrary pointers.
 * Добавить guarded strong root/edge; без retain/mark/destructor либо arbitrary pointers. */
int dvm_debug_census_field(DebugCensus *census, void *cell, int root);


/* [GC:] EN/RU: Scan immutable GC map within rooted bounded data extent.
 * Читать immutable GC map внутри rooted bounded data extent. */
int dvm_debug_census_map(DebugCensus *census, Type *type, unsigned char *data, size_t extent, int root);


/* [GC:] EN/RU: Traverse unique managed graph with all incoming references guarded.
 * Обойти unique managed graph с guarded incoming references. */
void dvm_debug_census_graph(DebugCensus *census);


/* [GC:] EN/RU: Compute on demand from frozen published roots; return bounded native JSON.
 * Вычислить по запросу из frozen published roots; вернуть bounded native JSON. */
void dvm_debug_census(DebugTarget *target, char *output, size_t capacity);
#endif
