# GC safe points / Безопасные точки GC

The collector traces active AVM1 operand/register/local/receiver roots, API stack
frames and registered native captures. Scheduling defers collection while the
native-call barrier is active; explicit [fvm_gc_collect](../include/flashvm/gc.h#L44) rejects that state.
Callbacks must return or leave a protected native segment before collection.
C clients retaining objects across collection need pins or scoped roots.
Native string results are retained in context storage; the existing arena is
released on context teardown, not by object sweeping (GC_STRING_STORAGE.md).

Сборщик сканирует корни активных AVM1 frames, API stacks и registered native
captures. Планировщик откладывает collection при активном native barrier;
явный [fvm_gc_collect](../include/flashvm/gc.h#L44) в таком состоянии отклоняется. Для хранения объектов
C-кодом через collection нужны pin или root scopes. Нативные строковые
результаты сохраняются в context arena, освобождаемом при уничтожении context.

[fvm_gc_step](../include/flashvm/gc.h#L90) выполняет threshold-driven operation; [fvm_gc_collect](../include/flashvm/gc.h#L44) — явный
проход. [FVM_GC_FULL](../include/flashvm/gc.h#L26) не меняет выбранный режим. Контекст используется только
с его VM thread. Детали: GC_AVM1_SAFE_POINTS.md, GC_CALLBACK_SAFETY.md,
GC_STRING_STORAGE.md. Полная приёмка GC и стадии 7 остаётся открытой.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:44](../include/flashvm/gc.h#L44) |
| `FVM_GC_FULL` | macro / define | [include/flashvm/gc.h:26](../include/flashvm/gc.h#L26) |
| `fvm_gc_step` | function | [include/flashvm/gc.h:90](../include/flashvm/gc.h#L90) |
<!-- source-links:end -->
