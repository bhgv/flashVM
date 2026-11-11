/* Copyright (c) 2026 bhgv
 * EN: Rooted emergency-string FIFO shared by reserve lifecycle and Fast GC.
 * RU: FIFO резервных строк с корнями для жизненного цикла резерва и Fast GC.
 */
#ifndef DVM_EXCEPTION_QUEUE_H
#define DVM_EXCEPTION_QUEUE_H
#include "disvm/native_world.h"

struct String; /* Managed string identity; layout is supplied by interp.h. */


/* [GC:] EN: Own a managed reference and its root until Fast removes both.
 * RU: Владеть управляемой ссылкой и её корнем до освобождения в Fast. */
typedef struct DVM_ExceptionRetirement
{
    struct DVM_ExceptionRetirement *next; /* Native retirement FIFO link. */
    struct String *message;               /* Rooted managed value. */
    DVM_NativeWorld *world;               /* Borrowed through the Fast worker join. */
    unsigned long epoch;                  /* Reader cutoff captured before FIFO publication. */
} DVM_ExceptionRetirement;


/* [GC:] EN: Borrow the FIFO head on its sole Fast consumer. Producers only append;
 * the head and immutable fields remain valid until that consumer takes/frees it.
 * Return NULL for an empty queue. The native FIFO mutex may briefly block.
 * RU: Получить голову FIFO на единственном потребителе Fast. Производители лишь
 * добавляют записи; голова живёт до take/free этого потребителя. При пустой
 * очереди вернуть NULL. Допустимо короткое ожидание нативного мьютекса.
 */
DVM_ExceptionRetirement *dvm_exception_retirement_peek(void);


/* [GC:] EN: Withdraw exactly entry on Fast, retaining its string/root ownership.
 * Abort on a consumer protocol violation. Caller later removes the root and frees.
 * RU: На Fast снять именно entry, сохранив владение строкой/корнем. При нарушении
 * протокола аварийно завершиться. Корень и память затем освобождает вызывающий.
 */
void dvm_exception_retirement_take(DVM_ExceptionRetirement *entry);


/* [GC:] EN: Count one shared release on Fast; peer records active guest overlap.
 * Read counters only after collector join. No managed storage is accessed here.
 * RU: Учесть shared-освобождение на Fast; peer означает активного гостя.
 * Читать счётчики после join сборщика. Управляемая память не затрагивается.
 */
void dvm_exception_retirement_count_shared(int peer);
#endif
