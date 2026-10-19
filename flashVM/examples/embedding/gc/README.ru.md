# Встраивание C89: трассирующий GC

Пример создаёт объект, удерживает его через [`fvm_gc_pin`](../../../include/flashvm/gc.h#L48), выполняет полный цикл сборки, снимает удержание и освобождает объект при следующей сборке. Указатель `FVM_Object *` заимствованный: нельзя обращаться к нему после освобождения сборщиком. Глобальный объект VM всегда является корнем.

Сборка CMake с `FVM_BUILD_EXAMPLES=ON`; запуск `flashvm_examples_embedding_gc_main`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `FVM_Object` | struct | [src/core/runtime.hpp:122](../../../src/core/runtime.hpp#L122) |
<!-- source-links:end -->
