/* Copyright (c) 2026 bhgv
 * EN: Immutable native launch data shared by explicitly related process constructors.
 * RU: Неизменяемые native launch data, разделяемые связанными process constructors.
 */
#ifndef DVM_LAUNCH_CONTEXT_H
#define DVM_LAUNCH_CONTEXT_H
#include "ticket.h"


/* EN/RU: Native-only copied data; references are atomic, payload immutable after creation.
 * Только native copied data; references атомарны, payload неизменяем после creation.
 */
typedef struct DVM_LaunchContext
{
    unsigned references; /* Native constructor/owner pins; never guest-visible or resurrected. */
    size_t argument_bytes; /* Exact immutable payload extent, bounded by launch admission. */
    char module[DVM_NS_PATH_LIMIT]; /* Copied logical label, never authority or resolved host path. */
    unsigned char arguments[]; /* Owned immutable binary payload, no native/managed pointers. */
} DVM_LaunchContext;


/* EN/RU: Copy borrowed exclusive handoff's label/bytes; owned context/NULL on invalid/OOM.
 * No I/O, authority acquisition or handoff mutation; caller pins input through return.
 * Скопировать label/bytes exclusive handoff; свой context/NULL при invalid/OOM.
 * Без I/O, получения authority и изменения handoff; caller удерживает input до возврата.
 */
DVM_LaunchContext *dvm_launch_context_create(const DVM_LaunchHandoff *handoff);


/* EN/RU: Add native pin to live borrowed context;1/0 on overflow/NULL, no allocation or I/O.
 * Добавить native pin живому borrowed context;1/0 при overflow/NULL, без allocation/I/O.
 */
int dvm_launch_context_retain(DVM_LaunchContext *context);


/* EN/RU: Consume one native pin, free only on final release; NULL allowed, no guest access/I/O.
 * Поглотить native pin, освободить при final release; NULL допустим, без guest access/I/O.
 */
void dvm_launch_context_release(DVM_LaunchContext *context);
#endif
