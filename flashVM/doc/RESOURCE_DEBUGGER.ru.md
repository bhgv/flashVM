# Команды отладчика и несколько наблюдателей

C89-адаптер `rdebug.h` регистрирует настоящий ресурс отладчика, например
`/debug/ctl`. Клиенты независимо наблюдают остановки и читают копии метаданных
стека. Изменять breakpoint и продолжать исполнение может один контроллер,
получивший права и занявший claim. [События](RESOURCE_EVENTS.ru.md),
[конверты](RESOURCE_DISPATCH.ru.md), [пример](../examples/debugger/resource_debugger/README.ru.md).

## API, время жизни и потоки

Context, dispatcher и userdata сервиса заимствуются и должны жить дольше адаптера.
VM-указатели и корни GC клиентам не передаются. Все действия с VM, регистрация,
отзыв и уничтожение выполняются владельцем. Producers используют только
синхронизированный диспетчер и кодеки. Hook устанавливается явно и снимается до
освобождения адаптера. Для нескольких потоков обязательны настоящие host lock/
unlock и проверка владельца; NULL policy означает однопоточное использование.

| Функция/тип | Контракт |
| --- | --- |
| [`fvm_rdebug_create`](../include/flashvm/rdebug.h#L73) | Context, dispatcher, путь, обязательный service, заимствованный userdata, два outputs. Выделяет native-память и регистрирует шесть операций без выдачи прав и установки hook. Ошибка не меняет outputs: INVALID для аргументов/пути, NOMEM при отказе памяти, остальные статусы регистрации передаются. |
| [`FVM_RDebugService`](../include/flashvm/rdebug.h#L68) | Синхронный вызов владельца после обработки доступных команд остановленной VM. [FVM_OK](../include/flashvm/flashvm.h#L42) — снова обработать очередь; другой [FVM_Result](../include/flashvm/flashvm.h#L40) — прервать исполнение. Реальный host ожидает транспорт/event loop с отменой и защитой от потерянного wakeup. Нельзя reenter/collect/destroy этого context и throw/longjmp через C. Потоки, сокеты, timeout и stdin не создаются. |
| [`fvm_rdebug_breakpoint`](../include/flashvm/rdebug.h#L83) | Устанавливается через `fvm_debug_set_breakpoint_handler(context, fvm_rdebug_breakpoint, adapter)`. Копирует сведения настоящей остановки, публикует STOP и обслуживает очередь до RESUME точного stop или ошибки service. Чужой context/рекурсия: INVALID_ARGUMENT; исчерпание stop/offset/line Uint32: UNSUPPORTED. Вызывать вручную или из транспортного потока нельзя. |
| [`fvm_rdebug_revoke`](../include/flashvm/rdebug.h#L79) | Владелец отзывает права сессии на этом ресурсе и её claim. Статусы диспетчера сохраняются. После остановки транспорта отдельно вызовите dispatcher_disconnect для остальных ресурсов/команд. Потеря контроллера не продолжает VM автоматически. |
| [`fvm_rdebug_destroy`](../include/flashvm/rdebug.h#L88) | Сначала снять VM hook и остановить пользователей. Во время остановки/обработчика: BUSY. Иначе unregister отзывает watch и старые команды, память освобождается, указатель обнуляется. Context и dispatcher не уничтожаются. |
| [`fvm_rdebug_snapshot_read`](../include/flashvm/rdebug.h#L91) | Клиентский декодер ответов SNAPSHOT и событий STOP/RESUME/ABORT. Проверяет канонические целые, Uint32, версию, состояние, флаги, число кадров, строки с длинами и точный конец. INVALID не меняет output. |
| [`fvm_rdebug_frame_read`](../include/flashvm/rdebug.h#L93) | Проверяет и копирует FRAME: версия, булевы поля, engine, длина имени, отсутствие хвоста. При ошибке output неизменен. Это копия метаданных, а не живая активация VM. |

## Операции и двоичные поля

Все целые, включая bool, версии и длины, записываются каноническими
неотрицательными S-integer. Имена — байты с длиной, без wire-терминатора NUL.
Внешний конверт — проверяемая S-запись. Чексумма и номера сессии/stop не заменяют
аутентификацию. Никакие присланные указатели C не вызываются.

| Операция | Права | Запрос и результат |
| --- | --- | --- |
| [FVM_RDEBUG_SNAPSHOT](../include/flashvm/rdebug.h#L10) | READ | Пустой запрос; последняя копия. До первой остановки RUNNING, stop=0, frames=0. VM не вызывается. |
| [FVM_RDEBUG_FRAME](../include/flashvm/rdebug.h#L11) | READ | `expected_stop, index`; кадр из кэша, в том числе после resume. Чужой stop: CONFLICT; индекс вне сохранённого префикса: UNKNOWN. |
| [FVM_RDEBUG_BREAKPOINT](../include/flashvm/rdebug.h#L12) | CONTROL + claim | `offset, enabled, name_size, name_bytes`; enabled 0/1, имя до [FVM_RDEBUG_NAME](../include/flashvm/rdebug.h#L7), без NUL. Пустое имя означает top-level. Ошибки VM переводятся в INVALID/NOMEM/HANDLER. |
| [FVM_RDEBUG_CLAIM](../include/flashvm/rdebug.h#L14) | CONTROL | Пустой запрос. Первый в owner FIFO занимает управление. Повтор той же сессии: OK, другая сессия: CONFLICT. Автоматических lease/timeout нет. |
| [FVM_RDEBUG_RELEASE](../include/flashvm/rdebug.h#L15) | CONTROL + claim | Пустой запрос; освобождает claim, но не продолжает VM. Чужая сессия: DENIED. |
| [FVM_RDEBUG_RESUME](../include/flashvm/rdebug.h#L17) | CONTROL + claim | `expected_stop`; вне callback: BUSY; старый/повторный stop: CONFLICT. OK означает намерение выхода из callback, а не уже исполненные инструкции. |

SNAPSHOT: `version, state, flags, stop, offset, line, frames, controller,
function_size, source_size, function_bytes, source_bytes`. FRAME:
`version, truncated, engine, forced, offset, argc, registers, function_size,
function_bytes`. Пределы: Uint32, 127 байт имени функции, 255 байт исходника,
16 внутренних кадров; обрезка помечается. Значения arguments/registers, locals,
объекты и адреса bytecode/machine code не экспортируются. Копии переживают GC
снаружи остановки и описывают свой stop, а не более поздний стек.

STOP/RESUME/ABORT содержат SNAPSHOT. Потери учитываются отдельно у каждого
наблюдателя. После потери нужно запросить актуальный снимок. Смена claim сама
по себе не публикует STOP: текущего контроллера узнают через SNAPSHOT. При потере
контроллера восстановлением управляет host. Ошибка service/публикации прерывает
callback; ABORT публикуется по возможности, состояние остаётся доступным для
запроса. Ошибка после принятого resume не доказывает продолжение программы.
Breakpoint использует существующий interpreter fallback, включая явно
прикреплённый JIT; deoptimization и асинхронная остановка не добавляются.

## События по двоичному интерфейсу

`fvm_revent_record_write(tag,event,output,capacity,used)` кодирует сохранённую
копию без чтения очереди. Поля: `version,target-slot,generation,sequence,kind,
missed,size,bytes`. Большой payload: S_LIMIT; неверные ID: S_INVALID. Ошибка не
меняет output/used; для повторной записи после сбоя I/O храните native-копию.

`fvm_revent_record_read(bindings,input,size,event,consumed)` требует входную
привязку `flashvm.resource.event`, version 1. Статусы CRC/framing сохраняются;
неизвестный тег: S_UNKNOWN, другая схема/неверное тело: S_INVALID. Outputs
меняются только при S_OK. Это не аутентификация и не выдача прав.

Аутентифицированный адаптер передаёт запросы в [`fvm_rdispatch_post_record`](../include/flashvm/rwire.h#L29) и
кодирует ответы/события. Реальный тест использует этот путь для команд и обеих
подписок. Сетевой listener, мост графов [FVM_Channel](../src/capi/channel_c.c#L27), Styx/9P и mount пока не
добавлены. Нельзя без разбора рассылать приватные сведения отладчика. Ресурсы
могут иметь разных владельцев; этот адаптер обслуживает ровно один context.

## Проверка

Команды сборки: [EN](RESOURCE_DEBUGGER.md#build-and-verification). Тест C89
исполняет настоящий AVM1, проверяет два независимых наблюдателя, кадры, конкуренцию
claim, старый/повторный resume, abort, отзыв и detach с ожидающими старыми
командами. Проверены interpreter/automatic-JIT/manual режимы; при доступном JIT
проверяется реальный native entry после снятия breakpoint. Отдельны oracle
событий Python и отказ выделения памяти адаптера. Target `flashvm_rdebug_threads`
дополнительно проверяет два настоящих клиентских потока, пока владелец VM
ожидает условную переменную. Этот тест доступен в нативной POSIX-сборке. Это не полный отладчик,
приёмка сетевого транспорта или нативный запуск на всех ОС.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Channel` | struct | [src/capi/channel_c.c:27](../src/capi/channel_c.c#L27) |
| `fvm_debug_set_breakpoint_handler` | function | [include/flashvm/debug.h:54](../include/flashvm/debug.h#L54) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_RDEBUG_BREAKPOINT` | macro / define | [include/flashvm/rdebug.h:12](../include/flashvm/rdebug.h#L12) |
| `fvm_rdebug_breakpoint` | function | [include/flashvm/rdebug.h:83](../include/flashvm/rdebug.h#L83) |
| `FVM_RDEBUG_CLAIM` | macro / define | [include/flashvm/rdebug.h:14](../include/flashvm/rdebug.h#L14) |
| `fvm_rdebug_create` | function | [include/flashvm/rdebug.h:73](../include/flashvm/rdebug.h#L73) |
| `fvm_rdebug_destroy` | function | [include/flashvm/rdebug.h:88](../include/flashvm/rdebug.h#L88) |
| `FVM_RDEBUG_FRAME` | macro / define | [include/flashvm/rdebug.h:11](../include/flashvm/rdebug.h#L11) |
| `fvm_rdebug_frame_read` | function | [include/flashvm/rdebug.h:93](../include/flashvm/rdebug.h#L93) |
| `FVM_RDEBUG_NAME` | macro / define | [include/flashvm/rdebug.h:7](../include/flashvm/rdebug.h#L7) |
| `FVM_RDEBUG_RELEASE` | macro / define | [include/flashvm/rdebug.h:15](../include/flashvm/rdebug.h#L15) |
| `FVM_RDEBUG_RESUME` | macro / define | [include/flashvm/rdebug.h:17](../include/flashvm/rdebug.h#L17) |
| `fvm_rdebug_revoke` | function | [include/flashvm/rdebug.h:79](../include/flashvm/rdebug.h#L79) |
| `FVM_RDEBUG_SNAPSHOT` | macro / define | [include/flashvm/rdebug.h:10](../include/flashvm/rdebug.h#L10) |
| `fvm_rdebug_snapshot_read` | function | [include/flashvm/rdebug.h:91](../include/flashvm/rdebug.h#L91) |
| `FVM_RDebugService` | callback typedef | [include/flashvm/rdebug.h:68](../include/flashvm/rdebug.h#L68) |
| `fvm_rdispatch_post_record` | function | [include/flashvm/rwire.h:29](../include/flashvm/rwire.h#L29) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../include/flashvm/flashvm.h#L40) |
| `fvm_revent_record_read` | function | [include/flashvm/rwire.h:49](../include/flashvm/rwire.h#L49) |
| `fvm_revent_record_write` | function | [include/flashvm/rwire.h:45](../include/flashvm/rwire.h#L45) |
<!-- source-links:end -->
