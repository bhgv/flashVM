/* Copyright (c) 2026 bhgv
 *
 * EN: Native snapshot data and interfaces for Cycles and Duke GC.
 * RU: Данные снимка графа и интерфейсы для Cycles и ДюкGC.
 */
#ifndef DVM_RC_GRAPH_SNAPSHOT_H
#define DVM_RC_GRAPH_SNAPSHOT_H

#include "disvm/range_access.h"
#include <stddef.h>

#define DUKE_INITIAL_FIELDS 2048 /* Initial shared snapshot capacity on the Duke lane. */
#define RECORD_SCRATCH_BYTES (4UL * 1024UL * 1024UL) /* Maximum Duke discovery storage. */
#define RECORD_EDGE_LIMIT 512 /* Maximum mapped fields in one guarded snapshot. */
#define RECORD_NODE_CAPACITY (RECORD_EDGE_LIMIT + 1) /* Seed plus one target per field. */
#define RECORD_INDEX_SLOTS (RECORD_EDGE_LIMIT * 4) /* Power of two, below half full. */

typedef struct RecordGroup
{
    void **nodes;             /* Captured object payloads. */
    unsigned int *incoming;   /* Internal incoming edge counts. */
    unsigned int *index;      /* Discovery hash slots, unused after capture. */
    unsigned int *dead_nodes; /* Claim order in a mixed published snapshot. */
    void **cells;             /* Captured source field addresses. */
    void **expected_fields;   /* Published values in field order, including nil. */
    unsigned int *targets;    /* Target node index or nil sentinel. */
    unsigned int *sources;    /* Source indices during discovery; NULL after publication. */
    unsigned char *live;      /* Nodes reachable from outside the group. */
    void *owned_scratch;      /* Duke's allocated discovery storage, or NULL. */
    unsigned int field_limit; /* Capture capacity of the backing storage. */
    size_t source_element;      /* Element cursor inside a suspended source. */
    size_t source_bit;          /* Next mapped bit within the current element. */
    int source_tail_seen;       /* List tail captured before its element fields. */
    unsigned int next_source;  /* Next source node in a suspended Duke scan. */
    unsigned int pinned_nodes; /* Duke holds nodes at indices 1 through this count. */
    int growing;             /* Duke can enlarge discovery storage under guards. */
    unsigned int dead_count;    /* Dead nodes in a published snapshot. */
    unsigned int live_count;    /* Nodes marked reachable. */
    int busy; /* A field stripe was temporarily owned elsewhere. */
    int oversized; /* Bounded discovery could not fit another field. */
    int has_views; /* A detached view needs descriptor-only destruction. */
    unsigned int nodes_used;    /* Indexed object count. */
    unsigned int edges_used;    /* Captured field count. */
    unsigned int omitted_live_fields; /* Live-source fields omitted from a published snapshot. */
} RecordGroup;


/* Scratch storage owned by one guarded discovery call. */
typedef struct RecordGroupScratch
{
    void *nodes[RECORD_NODE_CAPACITY];             /* Object payloads. */
    unsigned int incoming[RECORD_NODE_CAPACITY];  /* Internal incoming counts. */
    unsigned int index[RECORD_INDEX_SLOTS];      /* Node indices plus one. */
    void *cells[RECORD_EDGE_LIMIT];              /* Captured field addresses. */
    unsigned int targets[RECORD_EDGE_LIMIT];     /* Target indices. */
    unsigned int sources[RECORD_EDGE_LIMIT];     /* Source indices. */
    unsigned char live[RECORD_NODE_CAPACITY];      /* Reachability flags. */
} RecordGroupScratch;


/* EN: Use caller-owned zeroed scratch. RU: Использовать обнулённый буфер scratch. */
RecordGroup dvm_rc_empty_record_group(RecordGroupScratch *scratch);

/* EN: Count dead-source fields in group. Refuse a live-to-dead captured edge.
 * RU: Записать число полей мёртвых узлов group в edges. Отказать при ребре из живого узла в мёртвый. */
int dvm_rc_snapshot_edges(const RecordGroup *group, unsigned int *edges);

/* EN: Measure group storage for edges fields. RU: Измерить объём group для edges полей. */
size_t dvm_rc_record_group_bytes(const RecordGroup *group, unsigned int edges);

/* EN: Copy group with edges validated fields and nil_sentinel.
 * RU: Скопировать group с edges проверенными полями и значением nil_sentinel. */
RecordGroup *dvm_rc_copy_record_group(const RecordGroup *group, unsigned int edges, void *nil_sentinel);

/* EN: First hash probe for value in group. RU: Начальная позиция value в group. */
unsigned int dvm_rc_record_index_slot(const RecordGroup *group, void *value);

/* EN: Index payload value in group. RU: Найти или добавить адрес value в group. */
unsigned int dvm_rc_record_group_index(RecordGroup *group, void *value);

/* EN: Grow group for required fields. RU: Увеличить ёмкость group до required полей. */
int dvm_rc_grow_record_group(RecordGroup *group, size_t required);

/* EN: Reserve additional fields in group. RU: Зарезервировать fields мест в group. */
int dvm_rc_reserve_record_fields(RecordGroup *group, size_t fields);

/* EN: Read from rooted start using start_roots and borrowed access.
 * RU: Обойти граф от start с ожидаемым числом корней start_roots под защитой access. */
int dvm_rc_queued_record_group(void *start, unsigned long start_roots, RecordGroup *group,
    DVM_RangeAccess *access);

#define DVM_DUKE_CAPTURE_RESUME 2 /* The current slice ended with rooted work left. */

/* EN: Capture a slice from start into group under access. Return 2 to resume.
 * RU: Прочитать порцию графа start в group под access. Вернуть 2 для продолжения. */
int dvm_rc_duke_capture_slice(void *start, RecordGroup *group, DVM_RangeAccess *access);

#endif
