# EMBEDDING_API — русская документация

Создайте [`FVM_Config`](../include/flashvm/flashvm.h#L55), вызовите [`fvm_config_init`](../include/flashvm/flashvm.h#L70), затем [`fvm_runtime_create`](../include/flashvm/flashvm.h#L74) и [`fvm_context_create`](../include/flashvm/flashvm.h#L82). Загружайте SWF через [`fvm_load_memory`](../include/flashvm/flashvm.h#L90). При завершении сначала уничтожайте контекст, затем Runtime. [`fvm_advance`](../include/flashvm/flashvm.h#L94) пока не реализован. C API не содержит STL.

Подробные сигнатуры и ограничения см. в `EMBEDDING_API.md` и заголовках `include/flashvm/`. Примеры расположены в `examples/embedding/` и `examples/extending/`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_advance` | function | [include/flashvm/flashvm.h:94](../include/flashvm/flashvm.h#L94) |
| `FVM_Config` | struct | [include/flashvm/flashvm.h:55](../include/flashvm/flashvm.h#L55) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../include/flashvm/flashvm.h#L82) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:90](../include/flashvm/flashvm.h#L90) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../include/flashvm/flashvm.h#L74) |
<!-- source-links:end -->
