# GC safe points / Безопасные точки GC

The collector traces active AVM1 operand/register/local/receiver roots, API stack
frames and registered native captures. Scheduling defers collection while the
native-call barrier is active; explicit fvm_gc_collect rejects that state.
Callbacks must return or leave a protected native segment before collection.
C clients retaining objects across collection need pins or scoped roots.
Native string results are retained in context storage; the existing arena is
released on context teardown, not by object sweeping (GC_STRING_STORAGE.md).

Сборщик сканирует корни активных AVM1 frames, API stacks и registered native
captures. Планировщик откладывает collection при активном native barrier;
явный fvm_gc_collect в таком состоянии отклоняется. Для хранения объектов
C-кодом через collection нужны pin или root scopes. Нативные строковые
результаты сохраняются в context arena, освобождаемом при уничтожении context.

fvm_gc_step выполняет threshold-driven operation; fvm_gc_collect — явный
проход. FVM_GC_FULL не меняет выбранный режим. Контекст используется только
с его VM thread. Детали: GC_AVM1_SAFE_POINTS.md, GC_CALLBACK_SAFETY.md,
GC_STRING_STORAGE.md. Полная приёмка GC и стадии 7 остаётся открытой.
