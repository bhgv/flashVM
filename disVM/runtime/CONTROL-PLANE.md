# Диагностика, управление потоками и системная среда

Copyright (c) 2026 bhgv

Текущее состояние и контракт следующего этапа hosted DisVM, 2026-11-14.
Этот документ не объявляет перечисленные будущие API реализованными. Native
OS threads без global stop/exclusive остаются обязательным режимом.

## Что работает сейчас

| Область | Реализовано | Остаётся реализовать |
| --- | --- | --- |
| Гостевые потоки | Limbo spawn, permanent Prog/native owner; Processctl pid, kill/killwait/cancelwait для direct child | Supervisor capability для других гостей, pause/resume/step, приоритет |
| Наблюдение | Resident ID/PID, phase/activity, exact-ID progress, coherent registry summary | Location/stack, объекты ожиданий, external transport, диагностические mailbox |
| Память | Native allocator/RC/GC механизмы и private heap/type/module probes | Live allocator statistics и каталог объектов с owner/type attribution |
| Окружение | Guest-local native error storage и FP state | Copy/share policy namespace, FD groups, env и credentials |
| Файлы | Нативный файловый порт, open/create/read/write/seek/stat/fstat/dirread | Inferno namespace, bind/mount/unmount, работающий Styx serving и VFS providers |
| Модули | Limbo modules, typed C builtins по matched .m/NC64 | Общий .so/DLL loader, регистрация виртуальных FS/device providers |

Исходники контрактов: `module/processctl.m`, `runtime/process_environment.h`,
`runtime/prepare.py` (implemented Sys set), `runtime/NATIVE_MODULES.ru.md`.
Наличие декларации Sys/Debug/Styx либо upstream source не означает подключённый
сервис: неподдержанный Sys callback выдаёт явную ошибку. Текущие API служат
гостям VM; это не готовый внешний debugger/control endpoint.

## Отдельные диагностические mailbox

Согласовано с пользователем: запросы debug/stat идут отдельно от обычных channel,
RPC и completion messages. На каждого resident нужны собственная очередь запросов,
лимит памяти/числа записей и bounded ответы; медленный debugger получает явное
отклонение/усечение, а не задерживает приложение. Нужны отдельные control/event
квоты и fair bounded обслуживание. Diagnostics service имеет собственного owner
и продолжает отвечать при pause исследуемого гостя.

Запрос содержит VM/service generation, exact resident identity, request identity,
operation и capability. PID/адрес не заменяют capability. Ответ различает принято,
ожидание safe boundary, завершено, refused/denied, stale target и unavailable.
Отмена observer-запроса и его timeout не означают отмену guest operation. Нельзя
повторять mutating request после неизвестного исхода без проверки его identity.
Этот раздел не назначает новые публичные protocol IDs или wire formats.

Copied counters можно читать из native metadata. Stack/location/typed-variable
capture выполняет owner на разрешённой границе, с проверенным lifetime, type map
и bounded output. Getter, произвольный guest callback или remote RPC ради инспекции
не запускается. Общие объекты требуют object guards/revision checks даже тогда,
когда один Prog приостановлен: другие потоки продолжают менять shared heap.

## Pause/resume и управление

Pause — кооперативная остановка bytecode конкретного Prog на безопасной границе.
Permanent owner и live TLS REG сохраняются; чужой Prog не исполняется, а saved
image не становится источником возобновления. Диагностический wait loop владельца
должен обслуживать свой debug mailbox и wake/resume, не исполняя обычный bytecode.
GC получает корректные опубликованные roots и lifetime paused continuation.

Нельзя обещать немедленную pause внутри произвольного C callback, при удержании
object guard либо незавершённой native operation. Такой запрос остаётся pending
до допустимой границы либо получает явный отказ. Блокирующий native transport
нуждается в собственном cancellation/wakeup contract. Принудительный pthread
cancel/suspend не является реализацией этого контракта.

Kill, pause, resume, step и изменение приоритета — разные операции. Отдельный
supervisor capability может разрешить управление выбранным гостем; текущие
Processctl child permissions не расширяются молча. Priority задаётся через
порт платформы с явными supported/denied/unavailable результатами. Реализация
и проверки этого portable priority API пока отсутствуют. Native I/O/collector
helpers требуют собственной модели управления, а не гостевого PID.

Acceptance: другой Prog продолжает вычисления при pause; debugger отвечает,
roots/GC корректны; pending native wait, timeout, disconnect и target retirement
не теряют owner/ответы; resume продолжает тот же TLS REG. Shared heap не объявляется
замороженным. Broken mailbox/capability/generation controls должны обнаруживаться
семантически, без crashes/timeouts как критерия успеха.

## Учёт памяти

Следующие запросы должны отдельно показывать reserved/committed/live/retained/
free bytes, managed payload, allocator overhead и native metadata. Общий итог,
per-type, per-module и per-resident разрезы должны сходиться по известным правилам.

Нужно различать allocation origin и текущие retainers/roots: shared object нельзя
произвольно приписать одному потоку. Объект может пережить создавший его Prog.
Историческая origin identity имеет generation и не даёт управления новым гостем.
Для каталогов нужны stable allocation IDs, size/type, origin и lifetime-guarded
bounded projections; адрес не становится debug capability. Native buffers,
файлы/сокеты, registry slots и диагностические captures учитываются отдельно.

Existing heap_snapshot требует защищённого lifetime и frozen/excluded metadata
для части полей; это не готовый concurrent object enumeration. Сначала добавляется
безопасный metadata/counter API allocator, затем bounded каталоги и projections.

## Окружение и единая ФС

Нужна переносимая VM policy копирования/разделения namespace, descriptor groups,
env, credentials и других групп при spawn. Полный Linux clone ABI сейчас не
реализован и не переносится автоматически в semantics гостевого Prog. На Linux
kernel namespaces могут быть отдельной host capability, с собственными правами.

VFS должна дать per-environment namespace, composition/bind/mount/unmount и
провайдеры в C и Limbo. Файл, socket, device и файловый адаптер channel получают
общую модель handles/stat/read/write/control/close с cancellation и lifetime.
Typed VM channel сохраняет типы и message semantics; файловый адаптер явно задаёт
encoding/framing/backpressure, не выдавая layout heap за byte stream.

Провайдер имеет явные attach/open/read/write/stat/walk/close операции, bounded
requests, quotas и завершение pending операций при disconnect/unmount. Slow
provider не держит global lock и не останавливает других owners. Limbo provider
работает через typed mailbox, C provider — через проверенный native boundary.
Динамическая загрузка C-кода, его доверие/ABI и регистрация providers — отдельные
контракты. Сейчас C builtins встраиваются/регистрируются в host; `.m` сам не
загружает `.so`/DLL и не устанавливает файловую систему.

## Последовательность

1. Завершить copied progress/registry statistics и их lifecycle acceptance.
2. Diagnostic mailboxes, owner-bound module/location/stack capture и capabilities.
3. Cooperative pause/resume/step и portable priority с отдельной приёмкой.
4. Allocator metadata/statistics и bounded object/type/origin catalog.
5. Environment groups и composable VFS, затем C/Limbo providers и Styx integration.

Каждый этап принимается в native concurrent C/JIT runtime с GC, завершением Prog,
backpressure и semantic omission controls. Текущая приёмка предыдущего этапа не
служит доказательством готовности следующего.
