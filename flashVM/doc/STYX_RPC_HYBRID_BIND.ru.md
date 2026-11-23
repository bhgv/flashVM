# fvm_styxrpc_hybrid_bind

[English](STYX_RPC_HYBRID_BIND.md) · [Объявление](../include/flashvm/styxrpc_hybrid.h) · [Реализация](../src/styxrpc/hybrid.c) · [Методы таблицы](STYX_RPC_HYBRID.ru.md)

Подключите `flashvm/styxrpc_hybrid.h` и дополнительную цель `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_hybrid_bind(FVM_Context *context, const char *global);
```

На владельце `context` зарегистрируйте вызываемый конструктор под непустым именем `global`. Имя используется во время регистрации. Каждый вызов без аргументов создаёт отдельную пустую таблицу. Конструктор работает локально, без сессии и ожидания сети. Объект и все его методы принадлежат вызывающему контексту. Удерживайте сохранённую таблицу под корнем GC как обычный объект FlashVM.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) означает, что функция зарегистрирована. Отсутствие контекста или пустое имя дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Остальные ошибки регистрации передаются вызывающему. Конструктор с аргументами возвращает [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47), при нехватке памяти — [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46). Декодированные из графа таблицы сразу получают свои методы и доступны без регистрации конструктора.

[Исполняемая проверка AVM1](../tests/styxrpc/hybrid.c)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_hybrid_bind` | function | [include/flashvm/styxrpc_hybrid.h:16](../include/flashvm/styxrpc_hybrid.h#L16) |
<!-- source-links:end -->
