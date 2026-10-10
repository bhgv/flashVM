# EMBEDDING_API — русская документация

Создайте `FVM_Config`, вызовите `fvm_config_init`, затем `fvm_runtime_create` и `fvm_context_create`. Загружайте SWF через `fvm_load_memory`. При завершении сначала уничтожайте контекст, затем Runtime. `fvm_advance` пока не реализован. C API не содержит STL.

Подробные сигнатуры и ограничения см. в `EMBEDDING_API.md` и заголовках `include/flashvm/`. Примеры расположены в `examples/embedding/` и `examples/extending/`.
