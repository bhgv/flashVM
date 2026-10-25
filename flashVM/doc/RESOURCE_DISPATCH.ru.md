# Ограниченный dispatcher ресурсов владельца

[English](RESOURCE_DISPATCH.md) · [План ФС](RESOURCE_NAMESPACE.ru.md) · [Двоичные записи](SRECORD.ru.md).

Реализована очередь скопированных control-запросов, явные права и исполнение FIFO
у владельца. VM/SDL/transport не запускаются. Ресурсы пока представлены точными
абсолютными путями листьев; каталогов, aliases, subtree export и mount здесь нет.

Handle содержит slot ID и 32-битное поколение. Unregister увеличивает поколение
без wrap; исчерпанный slot выводится из использования. Старые queued handles дают
UNKNOWN и не вызывают новый userdata. Handle локален для lifetime dispatcher;
новый dispatcher/сеанс требует нового namespace epoch. Сам ID не является правом.

Сервер копирует до восьми описаний операций с обязательным READ/WRITE/CONTROL/
SUBSCRIBE. Клиент не выбирает требуемое право. Grant связывает поколение ресурса
с аутентифицированной адаптером сессией. Проверка выполняется при post и перед
исполнением. Отзыв останавливает ещё не начатое, но не откатывает running callback.
Session ID, переданный C API, доверен только после проверки адаптером; число из
сетевого пакета само по себе не аутентифицирует клиента.

## Владение и потоки

Policy копируется. Без lock/unlock всё используется одним владельцем/с внешней
сериализацией. С рабочим mutex хоста producers могут post/cancel/take одновременно
с одним owner pump. Обе lock-функции обязательны вместе, не отказывают и не входят
повторно в dispatcher. Необязательный is_owner запрещает чужим потокам register/
unregister/grant/pump/destroy. Без него исполнение у владельца — предусловие хоста.
Policy userdata/mutex живут до destroy; перед free необходимо остановить всех
native callers. Это не механизм автоматического удержания raw pointer.

Handler вызывается без queue lock. Register заимствует userdata; unregister
возвращает BUSY при исполнении handler. Queued запрос сохраняет лишь handle и
копию bytes, поэтому ещё не начатую регистрацию можно удалить. [GC:] Register
не pin-ит managed object. VM-адаптер должен удержать root у владельца и освободить
его после успешного unregister/destroy. Указатели VM не передаются в payload.

Callback может post-ить другую работу/читать ответы. Вложенный pump и destroy
дают BUSY. Возврат callback означает завершение его собственного контракта, не
обязательно показ кадра или распределённый commit. Исключения/longjmp через C и
запись сверх capacity запрещены. Неверный возвращённый size/status даёт HANDLER;
уже сделанный эффект при этом не откатывается.

## Очередь и ответы

Лимиты: 16 ресурсов, 64 grant, 32 принятых запроса, 512 bytes input/output.
Это control plane, не media buffer. Queued/running/done используют общую ёмкость.
Переполнение post даёт LIMIT без эффектов. До take сохраняется session/request ID;
повтор даёт CONFLICT. **После take тот же ID может исполниться снова.** Транспорт
отдельно задаёт ack/replay retention; exactly-once при crash/reconnect не обещается.

Post OK — принятие, не исполнение. Pump выполняет до budget FIFO callback;
их ошибки возвращаются в replies. Cancel удаляет queued из FIFO и создаёт
CANCELLED; running даёт BUSY, done — CONFLICT. Take копирует/удаляет ответ,
PENDING/UNKNOWN/ошибка сохраняют output. Destroy даёт BUSY, пока не забраны
все принятые ответы. Userdata/mutex хоста dispatcher не освобождает.

| C API | Назначение и публикация |
| --- | --- |
| [`fvm_rdispatch_create`](../include/flashvm/rdispatch.h#L109) | Validate/copy policy and allocate one bounded native object; publish output only on OK, NOMEM otherwise. |
| [`fvm_rdispatch_register`](../include/flashvm/rdispatch.h#L115) | Owner copies canonical path/operation table, borrows handler userdata and returns a fresh handle. |
| [`fvm_rdispatch_unregister`](../include/flashvm/rdispatch.h#L123) | Owner releases registration/grants; executing handler BUSY, queued old handles become stale. |
| [`fvm_rdispatch_grant`](../include/flashvm/rdispatch.h#L128) | Owner creates/replaces exact session authority; zero revokes and frees an entry. |
| [`fvm_rdispatch_lookup`](../include/flashvm/rdispatch.h#L133) | Exact path lookup with current authority; copy handle only on OK. No ambient filesystem lookup. |
| [`fvm_rdispatch_post`](../include/flashvm/rdispatch.h#L139) | Producers copy a bounded request after authority/identity/capacity checks. No handler runs here. |
| [`fvm_rdispatch_pump`](../include/flashvm/rdispatch.h#L145) | Owner delivers at most budget callbacks unlocked and retains their copied results. No recursive pumping. |
| [`fvm_rdispatch_cancel`](../include/flashvm/rdispatch.h#L149) | Session-scoped queued cancellation; does not interrupt a callback. |
| [`fvm_rdispatch_take`](../include/flashvm/rdispatch.h#L156) | Copy and acknowledge completed result; releases identity and queue capacity. |
| [`fvm_rdispatch_destroy`](../include/flashvm/rdispatch.h#L163) | Quiesced owner frees only dispatcher when no accepted requests remain; nulls caller pointer. |


Полные аргументы и lifetime для каждого API указаны в C89-заголовке и English
таблице выше. Пути сравниваются побайтно с учётом регистра; запрещены пустые
segments, dot/dot-dot, backslash/control bytes и trailing slash. Non-ASCII bytes
не нормализуются/не проверяются как UTF-8; отображение в native ФС не подразумевается.
Storage доступно и не перекрывает внутреннюю память dispatcher. Native structs
не передаются копированием sizeof на wire.

## Сборка

```sh
cmake -S flashVM -B build-resource -DFVM_BUILD_TESTS=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs"
cmake --build build-resource --target flashvm_srecord flashvm_srecord_oracle flashvm_rdispatch_contract --parallel
ctest --test-dir build-resource -R '^flashvm_rdispatch_(contract|wire)$' --output-on-failure
```

SDL/network SDK не требуется. Cross задаёт target toolchain/libc; тесты запускаются
на target. Native OS/device/VM/GUI integration и Inferno/9P conformance открыты.
## Двоичные конверты и реальные producers (v0.727)

[rwire.h](../include/flashvm/rwire.h) соединяет очередь с S-записями, не копируя
native structs. Сначала тег направления объявляется как flashvm.resource.request/
version 1 либо flashvm.resource.reply/version 1. Все числа — канонические
big-endian S integers; payload остаётся непрозрачным, максимум 512 bytes.

```text
request: version, request-id, target-slot, generation, operation-id, size, bytes
reply:   version, request-id, handler-result, size, bytes
```

**Session ID на wire нет:** post_record получает проверенную идентичность от
адаптера отдельно. Дополнительное поле пакета не позволяет выбрать чужую сессию.
Operation ID ресурса не является номером S-тега. CRC, имя типа и версия схемы
проверяются до queue post. Неизвестная привязка даёт UNKNOWN, неверные metadata/
CRC — INVALID, неполный префикс — PENDING; handler не вызывается. Автоматического
принятия деклараций типов нет.

| API | Назначение и владение |
| --- | --- |
| [`fvm_rrequest_record_write`](../include/flashvm/rwire.h#L23) | Пишет ограниченный request в буфер с выбранным тегом; session намеренно не кодирует. Очередь не меняется; S status и атомарный output. |
| [`fvm_rdispatch_post_record`](../include/flashvm/rwire.h#L33) | Проверяет запись/тип/body, затем копирует с authenticated session. Consumed меняется только при принятом post. После отказа транспорта требуется явный discard/ответ, не бесконечный повтор. |
| [`fvm_rreply_record_write`](../include/flashvm/rwire.h#L40) | Сериализует свою native-копию reply; результат очереди сам не забирает. Копию сохраняют для повторов output/I/O. |
| [`fvm_rreply_record_read`](../include/flashvm/rwire.h#L48) | Проверяет reply binding/schema, копирует id/result/bytes только при OK. На ошибке все выходы прежние, callback/queue не затрагиваются. |

Socket/auth/reconnect dedup/subscriptions/cancel packets/9P ещё не реализованы.
Native cancellation остаётся явным доверенным C-вызовом. Транспорт владеет
framing/consumption полной записи и временем жизни направленных таблиц.

Отдельный POSIX-тест запускает четыре настоящих producer threads и owner loop:
200 copied requests, host mutex, явный backpressure/ack. Workers не могут pump.
Handler повторно берёт nonrecursive mutex, проверяя освобождение lock перед
вызовом. Это native Linux threading, не проверка foreign OS/distributed mutex.
## Адаптер владельца плеера (v0.728)

[resource_control.h](../frontend/common/resource_control.h) регистрирует явный
ресурс наподобие /timeline/ctl с PAUSE/RESUME/STEP/STOP и правом CONTROL.
Хост выдаёт grants, устанавливает notify в обычный playback control и сохраняет
adapter/dispatcher до возврата runner. Init не запускает VM/окно/worker.

| Player API | Контракт |
| --- | --- |
| [fvm_desktop_resource_control_init](../frontend/common/resource_control.h#L29) | Владелец копирует путь/операции, заимствует storage adapter, публикует его только при успехе. Перед освобождением storage нужен unregister adapter.target. |
| [fvm_desktop_resource_control_notify](../frontend/common/resource_control.h#L39) | Сигнатура обычного playback notify. Проверяет READY/TURN/TICK, pump-ит максимум один FIFO job и возвращает native intent. Неверное состояние/ошибка pump даёт STOP. Context/root не сохраняет/не сериализует. |

Доставлять операции адаптера разрешено только внутри этого notify. Внешний pump
завершает их BUSY без изменения player policy. Payload команды пуст; неверный
payload даёт reply INVALID и KEEP. Reply OK подтверждает доставку в callback,
**до** применения player intent. Завершённое продвижение подтверждается поздним
TICK/frame. STEP сохраняет прежнюю семантику coalesced step permit: несколько
команд до такта не накапливают произвольное число кадров/инструкций. Для точного
счёта надо дождаться такта перед следующим STEP. STOP остаётся FIFO, не новым
priority interrupt; длинный owner handler принудительно не прерывается.

Адаптер не владеет dispatcher/grants/requests/registration/SDK/managed roots.
После возврата runner остановить внешних producers, cancel/take остатки, затем
unregister, destroy dispatcher, shutdown SDK и освобождение host storage. Перед
unload модуля с handler code также нужен unregister. Произвольные регистрации
хоста не добавляют неявный SDK shutdown guard. Прежние ограничения callbacks/
runners сохраняются.

[Capture tutorial](../examples/player/frame_capture/README.ru.md) получает
--resource-control: локальный native STOP после копии нужного кадра, без
обязательного кодирования. Отдельные live fixtures передают pause/step/stop
через привязанные S-records и проверяют реальные SWF pixels/paused refresh/
root navigation в interpreter/JIT с file/memory/HTTP. Это первая граница сервиса,
не перевод всех graphics/audio/clip классов и не remote GUI/socket/mount.
Handlers are synchronous: returning PENDING is rejected as HANDLER; terminal wire replies reject PENDING too.
Обработчик синхронен: возврат PENDING даёт HANDLER; terminal wire reply также не допускает PENDING.
## Player SDK build / Сборка адаптера плеера

```sh
cmake -S flashVM -B build-resource-player -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON -DFVM_PLAYER_SOCKET_HTTP=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DSDL2_DIR=/path/to/target-sdl/lib/cmake/SDL2
cmake --build build-resource-player --target flashvm_frontend_resource_control \
  flashvm_frontend_resource_player flashvm_frontend_host_abi flashvm_example_frame_capture --parallel
ctest --test-dir build-resource-player -R '^flashvm_resource_|^flashvm_frontend_host_abi$' --output-on-failure
```

POSIX native threading fixture / Только для native POSIX, не Windows:

```sh
cmake --build build-resource --target flashvm_rdispatch_threads --parallel
ctest --test-dir build-resource -R '^flashvm_rdispatch_threads$' --output-on-failure
```

Для cross задайте CMAKE_TOOLCHAIN_FILE и SDK SDL/libc/network именно target.
Shared-хост использует FVM_PLAYER_HOST_SHARED=ON и runtime search path из
[desktop setup](DESKTOP_FRONTEND.ru.md). Plain C contract протокола/адаптера
cross-link-ится без SDL; это отдельное свидетельство, не полный target player SDK
или запуск устройства. Live fixture дополнительно возобновляет timeline, затем
передаёт binary STOP при активном медленном HTTP child body и требует cancel/join.
Без HTTP provider проверяются local/embedded, а HTTP явно помечается недоступным.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_rdispatch_cancel` | function | [include/flashvm/rdispatch.h:149](../include/flashvm/rdispatch.h#L149) |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:109](../include/flashvm/rdispatch.h#L109) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:163](../include/flashvm/rdispatch.h#L163) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:128](../include/flashvm/rdispatch.h#L128) |
| `fvm_rdispatch_lookup` | function | [include/flashvm/rdispatch.h:133](../include/flashvm/rdispatch.h#L133) |
| `fvm_rdispatch_post` | function | [include/flashvm/rdispatch.h:139](../include/flashvm/rdispatch.h#L139) |
| `fvm_rdispatch_post_record` | function | [include/flashvm/rwire.h:33](../include/flashvm/rwire.h#L33) |
| `fvm_rdispatch_pump` | function | [include/flashvm/rdispatch.h:145](../include/flashvm/rdispatch.h#L145) |
| `fvm_rdispatch_register` | function | [include/flashvm/rdispatch.h:115](../include/flashvm/rdispatch.h#L115) |
| `fvm_rdispatch_take` | function | [include/flashvm/rdispatch.h:156](../include/flashvm/rdispatch.h#L156) |
| `fvm_rdispatch_unregister` | function | [include/flashvm/rdispatch.h:123](../include/flashvm/rdispatch.h#L123) |
| `fvm_rreply_record_read` | function | [include/flashvm/rwire.h:48](../include/flashvm/rwire.h#L48) |
| `fvm_rreply_record_write` | function | [include/flashvm/rwire.h:40](../include/flashvm/rwire.h#L40) |
| `fvm_rrequest_record_write` | function | [include/flashvm/rwire.h:23](../include/flashvm/rwire.h#L23) |
<!-- source-links:end -->
