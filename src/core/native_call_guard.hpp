/* FlashVM: Interpreter state, object lifetimes and shared runtime helpers: native call guard.
 * File: src/core/native_call_guard.hpp.
 */

/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#ifndef FVM_NATIVE_CALL_GUARD_HPP
/* Define FVM NATIVE CALL GUARD HPP constant or compilation switch. */
/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#define FVM_NATIVE_CALL_GUARD_HPP
/* [GC:] Подключаем объявления, используемые сборщиком и защитой корней. */
#include "runtime.hpp"
/* [GC:] User native callback entry commits an armed finalizer attempt. */
#include "finalizer_entry.hpp"
/* [GC:] C++98 scope guard: native callbacks must not reenter the collector.
   A callback may explicitly root objects and collect after it returns. */
/* [GC:] Define FVM_NativeCallGuard class and its representation. */
/* [GC:] Объявляем представление состояния сборщика или корней. */
class FVM_NativeCallGuard
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    FVM_Context *context_; /* [GC:] Owning interpreter context. */
    /* Prevent collector reentry while a native callback is active. */
    /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик. */
    FVM_NativeCallGuard(const FVM_NativeCallGuard &);
    /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик. */
    FVM_NativeCallGuard &operator=(
        /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик. */
        const FVM_NativeCallGuard &); /* Copy assignment is unavailable for this call guard. */

    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
  public:
    /* Prevent collector reentry while a native callback is active. */
    /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик. */
    explicit FVM_NativeCallGuard(FVM_Context *context) : context_(context)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Включаем барьер сборки на время нативного вызова. */
        ++context_->gc_native_depth;
        /* [GC:] Preparing roots may fail earlier; the native body starts only after this guard. */
        fvm_finalizer_enter(context_);
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Release the resources owned by this FVM_NativeCallGuard instance. */
    /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик. */
    ~FVM_NativeCallGuard()
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Снимаем барьер сборки после выхода из нативного вызова. */
        --context_->gc_native_depth;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
};
/* [GC:] Ограничиваем компиляцию объявлений и режимов сборщика. */
#endif
