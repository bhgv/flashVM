# fvm_debug_step_action

[English](DEBUG_STEP_ACTION.md) · [API отладчика](DEBUG_API.ru.md) · [Реализация](../src/jit/breakpoint.cpp)

```c
#include <flashvm/debug.h>
FVM_Result fvm_debug_step_action(FVM_Context *context);
```

Вызовите из активного синхронного колбэка точки останова в потоке
владельца. `context` заимствуется. Успех возвращает [`FVM_OK`](../include/flashvm/flashvm.h#L44) и ставит
одну остановку перед следующим действием интерпретатора, в том числе
перед действием окончания. Колбэк должен вернуть [`FVM_OK`](../include/flashvm/flashvm.h#L44), чтобы ход
выполнения дошёл до этой границы. Ошибка или замена колбэка снимает
ожидание шага. Вне колбэка возвращается [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47).
Вызов сам не выполняет действия и не переключает движок.
[Парный плеер](STYX_RPC_DEBUG.ru.md) выбирает интерпретатор перед
подключением отладчика и использует этот вызов для команды `step` в `.state`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:259](../src/core/runtime.hpp#L259) |
| `fvm_debug_step_action` | function | [include/flashvm/debug.h:74](../include/flashvm/debug.h#L74) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
<!-- source-links:end -->
