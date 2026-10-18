/* [GC:] FlashVM: Public embedding API declarations: gc roots.
 * File: include/flashvm/gc_roots.h.
 */

/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#ifndef FLASHVM_GC_ROOTS_H
/* Define FLASHVM GC ROOTS H constant or compilation switch. */
/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#define FLASHVM_GC_ROOTS_H
/* [GC:] Подключаем объявления, используемые сборщиком и защитой корней. */
#include "flashvm/value.h"
/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#ifdef __cplusplus
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C"
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#endif
    /* Temporary native roots. Scopes are strictly LIFO and context-local.
     * Values are copied, not referenced. Call end before destroying the context.
     * Never share a scope across threads. */
    /* [GC:] Объявляем представление состояния сборщика или корней. */
    typedef struct FVM_GCRootScope FVM_GCRootScope;
    /* Implement gc roots begin for this subsystem. */
    /* [GC:] Открываем область временных корней для защищаемых значений. */
    FVM_API FVM_Result fvm_gc_roots_begin(FVM_Context *, FVM_GCRootScope **);
    /* Copy a validated value into the active temporary root scope. */
    /* [GC:] Регистрируем значение в области временных корней. */
    FVM_API FVM_Result fvm_gc_roots_add(FVM_GCRootScope *, const FVM_Value *);
    /* Implement gc roots end for this subsystem. */
    /* [GC:] Закрываем область временных корней в обратном порядке открытия. */
    FVM_API FVM_Result fvm_gc_roots_end(FVM_GCRootScope *);
/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#ifdef __cplusplus
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}
/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#endif
/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#endif
