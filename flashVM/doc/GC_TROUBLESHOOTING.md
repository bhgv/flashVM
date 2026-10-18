# GC troubleshooting / Диагностика GC

If an object disappears unexpectedly, verify it was reachable from `global` or pinned using [`fvm_gc_pin`](../include/flashvm/gc.h#L48). Avoid retaining raw `FVM_Object*` beyond a collection without pinning.

Если объект неожиданно исчезает, проверьте его достижимость из `global` или закрепление [`fvm_gc_pin`](../include/flashvm/gc.h#L48). Не храните голые `FVM_Object*` через сборку без закрепления.

If memory is not reclaimed, inspect [`fvm_gc_object_count`](../include/flashvm/gc.h#L57), native pin counts and cycles reachable from roots. A cycle is not garbage if a live root points into it.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_gc_object_count` | function | [include/flashvm/gc.h:57](../include/flashvm/gc.h#L57) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../include/flashvm/gc.h#L48) |
| `FVM_Object` | struct | [src/core/runtime.hpp:119](../src/core/runtime.hpp#L119) |
<!-- source-links:end -->
