# Отладчик через ресурсные команды

Настоящая AVM1-функция возвращает 42 после остановки, двух независимых
наблюдателей и продолжения через owner FIFO. Только первая сессия имеет CONTROL.
Консоль лишь показывает результат; основной интерфейс — C89.

```sh
cmake -S flashVM -B build-debug -DFVM_BUILD_EXAMPLES=ON
cmake --build build-debug --target flashvm_examples_debugger_resource_debugger_main
./build-debug/flashvm_examples_debugger_resource_debugger_main
```

Нужны CMake и C89/C++98 toolchain; из компонента используйте `-S .`. VM без
рендера и SDL достаточна. Ожидаются два сообщения observer и итог result=42.
Host владеет runtime/context, диспетчером и native-адаптером; функция удерживается
корнем. Callback снимается до адаптера; сессии отключаются до dispatcher destroy.
Service здесь детерминированно ставит команды. Реальный транспорт должен ждать
событий с отменой, проверкой сессии, лимитами и защитой от lost wakeup. Нельзя
reenter/collect/destroy context из service. Сокеты, [FVM_Channel](../../../src/capi/channel_c.c#L30) graph bridge,
потоки, удалённая аутентификация и 9P не демонстрируются.

| API | Назначение и владение |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70) | Инициализирует конфигурацию до создания runtime. [Contract](../../../doc/API_INDEX.ru.md). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L82) | Создаёт context, используемый только потоком владельца. [Contract](../../../doc/API_INDEX.ru.md). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L86) | Освобождает VM после снятия callback и корня. [Contract](../../../doc/API_INDEX.ru.md). |
| [`fvm_debug_set_breakpoint`](../../../include/flashvm/debug.h#L61) | Владелец устанавливает breakpoint по offset; используется interpreter fallback. [Contract](../../../doc/RESOURCE_DEBUGGER.ru.md). |
| [`fvm_debug_set_breakpoint_handler`](../../../include/flashvm/debug.h#L66) | Явно ставит/снимает заимствованный callback; снять до освобождения userdata. [Contract](../../../doc/RESOURCE_DEBUGGER.ru.md). |
| [`fvm_function_create_avm1`](../../../include/flashvm/function.h#L62) | Копирует AVM1 в управляемую функцию; дальнейшее хранение требует корня. [Contract](../../../doc/API_INDEX.ru.md). |
| [`fvm_function_invoke`](../../../include/flashvm/function.h#L79) | Исполняет во владельце; результат 42 подтверждает работу после resume. [Contract](../../../doc/API_INDEX.ru.md). |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L60) | Удерживает функцию через host-вызовы и выделения памяти. [Contract](../../../doc/API_INDEX.ru.md). |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L70) | Снимает ровно ранее установленный host-корень. [Contract](../../../doc/API_INDEX.ru.md). |
| [`fvm_rdebug_create`](../../../include/flashvm/rdebug.h#L81) | Заимствует context, dispatcher и service; регистрирует ресурс без установки hook. [Contract](../../../doc/RESOURCE_DEBUGGER.ru.md). |
| [`fvm_rdebug_destroy`](../../../include/flashvm/rdebug.h#L102) | Удаляет ресурс и память после снятия hook; во время остановки BUSY. [Contract](../../../doc/RESOURCE_DEBUGGER.ru.md). |
| [`fvm_rdebug_snapshot_read`](../../../include/flashvm/rdebug.h#L107) | Декодирует копию события без заимствования объектов VM. [Contract](../../../doc/RESOURCE_DEBUGGER.ru.md). |
| [`fvm_rdispatch_create`](../../../include/flashvm/rdispatch.h#L109) | Создаёт ограниченный диспетчер; NULL hooks — один владелец. [Contract](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_destroy`](../../../include/flashvm/rdispatch.h#L163) | Освобождает память после остановки пользователей и очистки команд. [Contract](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_disconnect`](../../../include/flashvm/revents.h#L99) | Владелец отзывает сессию и удаляет её ожидающие команды/ответы. [Contract](../../../doc/RESOURCE_EVENTS.ru.md). |
| [`fvm_rdispatch_event_take`](../../../include/flashvm/revents.h#L79) | Копирует событие одного читателя, не забирая копии у других. [Contract](../../../doc/RESOURCE_EVENTS.ru.md). |
| [`fvm_rdispatch_grant`](../../../include/flashvm/rdispatch.h#L128) | Владелец явно выдаёт права конкретной сессии. [Contract](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_post`](../../../include/flashvm/rdispatch.h#L139) | Копирует запрос; выполнение позже во владельце, снаружи lock очереди. [Contract](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_subscribe`](../../../include/flashvm/revents.h#L63) | Создаёт независимую очередь при наличии SUBSCRIBE. [Contract](../../../doc/RESOURCE_EVENTS.ru.md). |
| [`fvm_rdispatch_take`](../../../include/flashvm/rdispatch.h#L156) | Копирует и подтверждает готовый ответ, освобождая слот. [Contract](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L74) | Создаёт runtime; владелец уничтожает его после context. [Contract](../../../doc/API_INDEX.ru.md). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L78) | Освобождает runtime после context и адаптера. [Contract](../../../doc/API_INDEX.ru.md). |
| [`fvm_sinteger_write`](../../../include/flashvm/srecord.h#L62) | Кодирует ожидаемый stop каноническим S-integer в буфер вызывающей стороны. [Contract](../../../doc/SRECORD.ru.md). |

[Source](main.c), [guide](../../../doc/RESOURCE_DEBUGGER.ru.md), [catalog](../../README.md#example-debugger-resource_debugger).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Channel` | struct | [src/capi/channel_c.c:30](../../../src/capi/channel_c.c#L30) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_debug_set_breakpoint` | function | [include/flashvm/debug.h:61](../../../include/flashvm/debug.h#L61) |
| `fvm_debug_set_breakpoint_handler` | function | [include/flashvm/debug.h:66](../../../include/flashvm/debug.h#L66) |
| `fvm_function_create_avm1` | function | [include/flashvm/function.h:62](../../../include/flashvm/function.h#L62) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:79](../../../include/flashvm/function.h#L79) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:60](../../../include/flashvm/gc.h#L60) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:70](../../../include/flashvm/gc.h#L70) |
| `fvm_rdebug_create` | function | [include/flashvm/rdebug.h:81](../../../include/flashvm/rdebug.h#L81) |
| `fvm_rdebug_destroy` | function | [include/flashvm/rdebug.h:102](../../../include/flashvm/rdebug.h#L102) |
| `fvm_rdebug_snapshot_read` | function | [include/flashvm/rdebug.h:107](../../../include/flashvm/rdebug.h#L107) |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:109](../../../include/flashvm/rdispatch.h#L109) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:163](../../../include/flashvm/rdispatch.h#L163) |
| `fvm_rdispatch_disconnect` | function | [include/flashvm/revents.h:99](../../../include/flashvm/revents.h#L99) |
| `fvm_rdispatch_event_take` | function | [include/flashvm/revents.h:79](../../../include/flashvm/revents.h#L79) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:128](../../../include/flashvm/rdispatch.h#L128) |
| `fvm_rdispatch_post` | function | [include/flashvm/rdispatch.h:139](../../../include/flashvm/rdispatch.h#L139) |
| `fvm_rdispatch_subscribe` | function | [include/flashvm/revents.h:63](../../../include/flashvm/revents.h#L63) |
| `fvm_rdispatch_take` | function | [include/flashvm/rdispatch.h:156](../../../include/flashvm/rdispatch.h#L156) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_sinteger_write` | function | [include/flashvm/srecord.h:62](../../../include/flashvm/srecord.h#L62) |
<!-- source-links:end -->
