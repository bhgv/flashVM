/* Copyright (c) 2026 bhgv
 * EN: Pointer-free owner location captures with bounded UTF-8 display labels.
 * RU: Снимки location владельца без указателей, с ограниченными UTF-8 подписями.
 */
#ifndef DVM_DIAGNOSTIC_LOCATION_H
#define DVM_DIAGNOSTIC_LOCATION_H

#define DVM_DIAG_NAME_BYTES 64  /* Native capacity including terminating NUL. */
#define DVM_DIAG_PATH_BYTES 256 /* Native capacity including terminating NUL. */

enum
{
    DVM_LOCATION_VALID = 1,          /* Checked original-Dis next-PC geometry and copied labels. */
    DVM_LOCATION_NAME_TRUNCATED = 2, /* Name ends at a complete UTF-8 prefix. */
    DVM_LOCATION_PATH_TRUNCATED = 4  /* Path ends at a complete UTF-8 prefix. */
};


/* EN: One copied boundary; labels do not identify a globally unique Module.
 * RU: Копия одной границы; подписи не идентифицируют глобально уникальный Module. */
typedef struct DVM_DiagnosticLocation
{
    unsigned long sequence;         /* Owner-local saturated boundary sequence, not module authority. */
    int next_pc;                    /* Original Dis next-PC index, possibly nprog at end. */
    int nprog;                      /* Instruction extent of the captured module. */
    int flags;                      /* Validity and UTF-8 truncation bits. */
    int name_bytes;                 /* Copied bytes excluding NUL. */
    int path_bytes;                 /* Copied bytes excluding NUL. */
    char name[DVM_DIAG_NAME_BYTES]; /* Copied module display name. */
    char path[DVM_DIAG_PATH_BYTES]; /* Copied module path; no retained pointer. */
} DVM_DiagnosticLocation;


struct Modlink;


/* [GC:] EN: Active owner borrows live rooted module and its own next PC. Output is
 * disjoint native storage. Invalid/native-PC geometry or invalid UTF-8 leaves
 * zero unavailable output. No allocation, callbacks, retention or target lookup.
 * RU: Active owner заимствует живой rooted module и свой next PC. Output —
 * отдельная native память. Неверная/native-PC geometry либо UTF-8 дают нулевой
 * unavailable output. Без allocation, callbacks, retention либо поиска target. */
void dvm_diagnostic_location_capture(DVM_DiagnosticLocation *output, struct Modlink *module, const void *next_pc);
#endif
