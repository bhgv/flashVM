# EMBEDDING_API — русская документация

Создайте [`FVM_Config`](../include/flashvm/flashvm.h#L51), вызовите [`fvm_config_init`](../include/flashvm/flashvm.h#L62), затем [`fvm_runtime_create`](../include/flashvm/flashvm.h#L64) и [`fvm_context_create`](../include/flashvm/flashvm.h#L68). Загружайте SWF через [`fvm_load_memory`](../include/flashvm/flashvm.h#L72). При завершении сначала уничтожайте контекст, затем Runtime. [`fvm_advance`](../include/flashvm/flashvm.h#L74) пока не реализован. C API не содержит STL.

Подробные сигнатуры и ограничения см. в `EMBEDDING_API.md` и заголовках `include/flashvm/`. Примеры расположены в `examples/embedding/` и `examples/extending/`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_advance` | function | [include/flashvm/flashvm.h:74](../include/flashvm/flashvm.h#L74) |
| `FVM_Config` | struct | [include/flashvm/flashvm.h:51](../include/flashvm/flashvm.h#L51) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../include/flashvm/flashvm.h#L68) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../include/flashvm/flashvm.h#L72) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../include/flashvm/flashvm.h#L64) |
<!-- source-links:end -->
