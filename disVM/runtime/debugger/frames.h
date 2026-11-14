/* Copyright (c) 2026 bhgv
 * EN: Private borrowed frame views under an acknowledged frozen source gate.
 * RU: Private borrowed frame views под подтверждённым frozen source gate.
 */
#ifndef DVM_DEBUG_FRAMES_H
#define DVM_DEBUG_FRAMES_H


/* [GC:] EN/RU: Borrow only through the inspector's current ACTIVE operation.
 * Заимствовать только до конца текущей ACTIVE операции inspector. */
typedef struct DebugFrame
{
    unsigned char *data; /* Frozen managed stack frame base. */
    Type *type;          /* Immutable descriptor retained by that frame/extent. */
    Modlink *module;     /* Rooted current/saved module owner. */
    const void *pc;      /* Original Dis next/return coordinate. */
} DebugFrame;


/* [GC:] EN/RU: Find index0..127 from frozen publication; no foreign live REG read.
 * Найти index0..127 из frozen publication; без чтения foreign live REG. */
int dvm_debug_frame(const DebugTarget *target, unsigned int index, DebugFrame *output);
#endif
