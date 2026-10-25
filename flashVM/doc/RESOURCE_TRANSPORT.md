# Передача ресурсов через файлы и TCP / Resource transport

## Русский

Файловый канал сохраняет адресный дескриптор живого мутекса/канала, а не копию его
нативного состояния. После восстановления операции обращаются к исходному
ресурсу по TCP. Дескриптор содержит адрес сервиса владельца, случайный идентификатор
сеанса и случайный токен ресурса. Перезапуск сервиса меняет сеанс: старый файл не
создаёт новый мутекс и не восстанавливает потерянную блокировку.

Хост запускает сервис через [`fvm_broker_start`](../include/flashvm/capability.h#L21), задавая адрес привязки, доступное
получателю имя/IP и порт (0 выбирает свободный). [`fvm_context_set_broker`](../include/flashvm/capability.h#L58)
присоединяет сервис к контексту; скриптовые рабочие потоки наследуют присоединение.
Сериализация локальных ресурсов в файл/сетевой канал требует сервиса, иначе
возвращается [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48). Импортированные сетевые дескрипторы можно
передавать дальше без собственного сервиса.

[`fvm_broker_export`](../include/flashvm/capability.h#L63) возвращает URI `fvm://host:port/session/token/8` для мутекса
или `/1` для канала; строку освобождает [`fvm_serialized_free`](../include/flashvm/serialization.h#L30).
[`fvm_broker_import`](../include/flashvm/capability.h#L67) возвращает объект ActionScript; [`fvm_channel_open_network`](../include/flashvm/capability.h#L91) —
удерживаемый нативный канал. В ActionScript используются
`channel.new("network", uri)` и `channel.new("file", path[, mode])`, где режим
`r`, `w` или `rw` (по умолчанию). URI передаётся скрипту обычной глобальной строкой.
Запуск слушающего сервиса пока доступен через C API.

Повторный импорт URI в контексте возвращает ту же обёртку; контекст с сервисом
владельца получает исходную локальную обёртку. Обычные графы значений сохраняют
циклы и общие ссылки. Поля и метатаблицы обёртки ресурса остаются локальными.

Удалённый `set` многократно пробует неблокирующий захват на стороне владельца и
ждёт при занятости. Сервис продолжает обслуживать другие запросы. `touch`
опрашивает исходный ресурс; `reset` проверяет владельца и освобождает его.
Владельцы различаются случайным идентификатором процесса и идентификатором
контекста. `origin` сетевой обёртки содержит URI. Уничтожение контекста делает
попытку освободить его удалённые захваты. `close` мутекса инвалидирует ресурс;
последующие операции/восстановление дескриптора возвращают ошибку.
Каналы передают графы, включая дескрипторы других каналов и мутексов. Заполненная
ограниченная очередь возвращает [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L49); блокирующая передача ждёт места.
Ожидания мутекса, пустого канала и заполненной очереди реагируют на остановку
рабочего потока.

Сервис удерживает экспортированные ресурсы до [`fvm_broker_stop`](../include/flashvm/capability.h#L50). Остановите
сервис перед освобождением своей ссылки [`fvm_broker_release`](../include/flashvm/capability.h#L54); присоединённые
контексты удерживают свои ссылки. Stop/release одного сервиса нельзя выполнять
одновременно. Runtime должен жить до уничтожения его контекстов.
Компилируемый C89-пример: `examples/embedding/resource_broker/main.c`.

С v0.8 ошибки AS-методов ресурсов выбрасывают `ResourceException` и возвращают
[`FVM_EXCEPTION`](../include/flashvm/flashvm.h#L50) приложению; нативные API сохраняют [`FVM_ERROR`](../include/flashvm/flashvm.h#L45). Подробности:
[исключения](EXCEPTIONS.ru.md). Ошибки также попадают в прежнюю очередь
[`fvm_debug_resource_errors`](../include/flashvm/debug.h#L199): до 64 копий событий с источником, строкой, функцией,
смещением, операцией и адресом. Переполнение удаляет старейшее событие. Это API
перехвата ошибок, а не готовый интерфейс остановки и пошагового исполнения.
Замораживание/просмотр сообщений выполняются на исходной очереди владельца
существующим API отладчика; удалённая команда freeze пока не реализована.

Границы текущей реализации:

- Bearer-токен URI предоставляет полномочия на ресурс. TLS нет: нужна доверенная
  сеть либо защищённый туннель.
- При потере ответа на `set` возвращается ошибка без предположения об успешном
  захвате и без автоматического повтора. Аренд, восстановления после аварии
  клиента и согласования неопределённого результата нет; остановка сервиса
  освобождает захваты его служебных контекстов.
- Распределённые дедлоки и карты исходников удалённых владельцев не анализируются.
  Локальный детектор не доказывает отсутствие сетевых дедлоков.
- Пользовательские callback-кодеки не регистрируются автоматически в служебных
  контекстах. Перекодирование памяти с такими объектами и ресурсами может быть
  отклонено; сами адресные дескрипторы передаются независимо от указателей.
- TCP-подключение ограничено двумя секундами на адрес, чтение/запись — двухсекундным
  таймаутом бездействия. DNS остаётся блокирующим вызовом платформы. Сервис
  обрабатывает запросы последовательно; неисправный клиент задерживает остальных.
- Удалённые `ready`/`pending`/изменение capacity, сырой файловый read/write и
  удалённый freeze пока не предоставлены; очередь настраивается у владельца.
- URI поддерживает DNS/IPv4, не литерал IPv6. Без [`OS_PORTS`](../cmake/BuildDefinitions.cmake#L11) API сервиса возвращает
  [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48). Проверки выполнены на Linux; Windows/macOS/Android требуют
  собственных сборок и запуска тестов.

## English

A file stores a live mutex/channel capability, never a copy of native state.
Restored operations target the original resource over TCP. A URI contains the
owner's address, random session and random resource token. Restarting the authority
invalidates old descriptors; files do not reconstruct lost lock state.

Start with `fvm_broker_start(bind_host, advertised_host, port, &broker)` and bind
with [`fvm_context_set_broker`](../include/flashvm/capability.h#L58); port zero selects a free port. Script workers
inherit the binding. Exporting local resources into file/network records requires
a broker, otherwise it returns [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48). Remote descriptors can be
forwarded without a local broker. [`fvm_broker_export`](../include/flashvm/capability.h#L63) returns a malloc-owned URI
(release using [`fvm_serialized_free`](../include/flashvm/serialization.h#L30)); [`fvm_broker_import`](../include/flashvm/capability.h#L67) returns an ActionScript
wrapper. [`fvm_channel_open_network`](../include/flashvm/capability.h#L91) returns a retained native channel. ActionScript
uses `channel.new("network", uri)` or `channel.new("file", path[, mode])` with
`r`, `w`, default `rw`. Listening is currently a host C API operation.

Repeated imports are canonical in a context; a context bound to the original
broker resolves its local wrapper. Surrounding graph cycles/aliases survive;
wrapper decorations/metatables remain local. Remote `set` polls nonblocking
acquisition at the authority while other requests continue. `touch` queries state;
`reset` verifies ownership using a client process nonce and context identity.
Remote `origin` contains the URI. Context teardown attempts to release remote
acquisitions. Mutex close invalidates the original resource. Network channels
transport graphs and resource descriptors with bounded-queue backpressure.
Mutex/empty/full queue waits observe worker cancellation.

Exported resources remain retained until [`fvm_broker_stop`](../include/flashvm/capability.h#L50); stop before releasing
the caller's reference. Bound contexts hold their own references. Do not race
stop/release. Keep runtimes alive until contexts are destroyed. The C89 example
is `examples/embedding/resource_broker/main.c`.

Closed/unreachable resource errors enter [`fvm_debug_resource_errors`](../include/flashvm/debug.h#L199), a 64-event
ring of copied source/function, line/offset, operation and address. Overflow drops
the oldest event. This is an interception API, not a complete pause/step UI.
Existing dequeue freeze/message inspection work at the authority's queue; remote
freeze, distributed deadlock analysis and remote source-map propagation are absent.

Bearer tokens grant resource authority; TLS is absent, requiring a trusted network
or secure tunnel. Lost acquisition responses return errors without assuming
success or replaying the request. Leases, crash recovery and uncertain-result
reconciliation remain absent; authority stop releases service-owned locks. Custom
callback codecs are not automatically registered in service contexts, so converting
in-process packets containing callbacks and resources may be rejected. Connect is
bounded to two seconds per address; socket reads/writes have two-second inactivity
timeouts. Platform DNS remains blocking. Requests are served sequentially.
Remote ready/pending/capacity changes, raw file read/write and freeze are not
provided; configure/inspect the queue at its owner. DNS/IPv4 URI literals are
supported, IPv6 literals are not. Without [`OS_PORTS`](../cmake/BuildDefinitions.cmake#L11),
broker APIs return [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48). Linux tests cover separate processes, file
restoration, ownership, resource forwarding, closure, contention and cancellation;
Windows/macOS/Android execution remains unverified.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_broker_export` | function | [include/flashvm/capability.h:63](../include/flashvm/capability.h#L63) |
| `fvm_broker_import` | function | [include/flashvm/capability.h:67](../include/flashvm/capability.h#L67) |
| `fvm_broker_release` | function | [include/flashvm/capability.h:54](../include/flashvm/capability.h#L54) |
| `fvm_broker_start` | function | [include/flashvm/capability.h:21](../include/flashvm/capability.h#L21) |
| `fvm_broker_stop` | function | [include/flashvm/capability.h:50](../include/flashvm/capability.h#L50) |
| `fvm_channel_open_network` | function | [include/flashvm/capability.h:91](../include/flashvm/capability.h#L91) |
| `fvm_context_set_broker` | function | [include/flashvm/capability.h:58](../include/flashvm/capability.h#L58) |
| `fvm_debug_resource_errors` | function | [include/flashvm/debug.h:199](../include/flashvm/debug.h#L199) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_EXCEPTION` | enum constant | [include/flashvm/flashvm.h:50](../include/flashvm/flashvm.h#L50) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:49](../include/flashvm/flashvm.h#L49) |
| `fvm_serialized_free` | function | [include/flashvm/serialization.h:30](../include/flashvm/serialization.h#L30) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
| `OS_PORTS` | DEFS switch | [cmake/BuildDefinitions.cmake:11](../cmake/BuildDefinitions.cmake#L11) |
<!-- source-links:end -->
