/* FlashVM: Public embedding API declarations: gc.
 * File: include/flashvm/gc.h.
 */

/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#ifndef FLASHVM_GC_H
/* Define FLASHVM GC H constant or compilation switch. */
/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#define FLASHVM_GC_H
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
/* GC is context-local; never call from another thread. */
/* Define FVM GC SUSPENDED constant or compilation switch. */
/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#define FVM_GC_SUSPENDED 101
/* Define FVM GC FULL constant or compilation switch. */
/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#define FVM_GC_FULL 102
    /* -100..100: collection frequency; 0: normal. */
    /* Implement gc mode for this subsystem. */
    /* [GC:] Работаем с выбранным режимом планирования сборки. */
    FVM_API FVM_Result fvm_gc_mode(FVM_Context *, int mode);
    /* Implement gc state for this subsystem. */
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    FVM_API FVM_Result fvm_gc_state(FVM_Context *, int *mode);
    /* Explicit full mark/sweep; counts are optional. */
    /* Mark roots, finalize unreachable objects, mark resurrected graphs and sweep. */
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    FVM_API FVM_Result fvm_gc_collect(FVM_Context *, unsigned long *reclaimed);
    /* Pin an object while its pointer is held by native code. */
    /* Implement gc pin for this subsystem. */
    /* [GC:] Работаем со счётчиком явных корней хоста для управляемого объекта. */
    FVM_API FVM_Result fvm_gc_pin(FVM_Context *, FVM_Object *);
    /* Read the number of explicit host roots retaining this object. */
    /* [GC:] Работаем со счётчиком явных корней хоста для управляемого объекта. */
    FVM_API FVM_Result fvm_gc_pin_count(FVM_Context *, FVM_Object *, unsigned long *count);
    /* Implement gc unpin for this subsystem. */
    /* [GC:] Снимаем одну явную удерживающую ссылку хоста. */
    FVM_API FVM_Result fvm_gc_unpin(FVM_Context *, FVM_Object *);
    /* Read the current number of managed objects in the context. */
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    FVM_API FVM_Result fvm_gc_object_count(FVM_Context *, unsigned long *);
    /* Number of object allocations since the last successful collection. */
    /* Implement gc pending allocations for this subsystem. */
    /* [GC:] Работаем с очередью объектов для итеративного обхода графа. */
    FVM_API FVM_Result fvm_gc_pending_allocations(FVM_Context *, unsigned long *count);
    /* Query the allocation threshold for automatic collection at a safe point. */
    /* Implement gc threshold for this subsystem. */
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    FVM_API FVM_Result fvm_gc_threshold(FVM_Context *, unsigned long *threshold);
    /* Snapshot of collector state. This is a diagnostic API, not a memory-size
       accounting API. struct_size permits future extension of the C89 ABI. */
    /* [GC:] Define FVM_GCStats struct and its representation. */
    /* [GC:] Объявляем представление состояния сборщика или корней. */
    typedef struct FVM_GCStats
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        unsigned int struct_size;   /* [GC:] Size of this structure for API compatibility checks. */
        unsigned long live_objects; /* [GC:] Number of live managed objects. */
        unsigned long pinned_objects; /* [GC:] Number of explicitly rooted objects. */
        /* [GC:] Работаем с очередью объектов для итеративного обхода графа. */
        unsigned long pending_allocations; /* Allocations pending a scheduled collection. */
        unsigned long threshold;   /* [GC:] Allocation threshold for scheduling collection. */
        int mode;                  /* [GC:] Selected operating or collector mode. */
        int collecting;            /* [GC:] Whether collection is currently running. */
        unsigned int native_depth; /* [GC:] Current native-call nesting depth. */
        /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    } FVM_GCStats;
    /* Snapshot collector mode, object counts, roots and allocation accounting. */
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    FVM_API FVM_Result fvm_gc_stats(FVM_Context *, FVM_GCStats *);
    /* Call at safe points. Never collect inside native callbacks. */
    /* Run a collection at a safe point when the scheduling threshold is reached. */
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    FVM_API FVM_Result fvm_gc_step(FVM_Context *, unsigned long *reclaimed);
    /* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
    /* [GC:] Exact context string-buffer accounting; excludes container and allocator overhead. */
    typedef struct FVM_GCStringStats
    {
        size_t buffers;         /* [GC:] Number of registered context-owned string allocations. */
        size_t payload_bytes;   /* [GC:] Counted bytes, preserving NUL and WTF-8 payload lengths. */
        size_t allocated_bytes; /* [GC:] Payload bytes plus one terminator per allocation. */
        /* [GC:] Complete the scalar string-storage snapshot type. */
    } FVM_GCStringStats;
    /* [GC:] Read string arena statistics without allocation; errors leave caller output unchanged.
     */
    FVM_API FVM_Result fvm_gc_string_stats(FVM_Context *, FVM_GCStringStats *);
#ifdef __cplusplus
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}
/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#endif
/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#endif
