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
| [`fvm_rdispatch_create`](../include/flashvm/rdispatch.h#L95) | Validate/copy policy and allocate one bounded native object; publish output only on OK, NOMEM otherwise. |
| [`fvm_rdispatch_register`](../include/flashvm/rdispatch.h#L99) | Owner copies canonical path/operation table, borrows handler userdata and returns a fresh handle. |
| [`fvm_rdispatch_unregister`](../include/flashvm/rdispatch.h#L105) | Owner releases registration/grants; executing handler BUSY, queued old handles become stale. |
| [`fvm_rdispatch_grant`](../include/flashvm/rdispatch.h#L108) | Owner creates/replaces exact session authority; zero revokes and frees an entry. |
| [`fvm_rdispatch_lookup`](../include/flashvm/rdispatch.h#L111) | Exact path lookup with current authority; copy handle only on OK. No ambient filesystem lookup. |
| [`fvm_rdispatch_post`](../include/flashvm/rdispatch.h#L115) | Producers copy a bounded request after authority/identity/capacity checks. No handler runs here. |
| [`fvm_rdispatch_pump`](../include/flashvm/rdispatch.h#L119) | Owner delivers at most budget callbacks unlocked and retains their copied results. No recursive pumping. |
| [`fvm_rdispatch_cancel`](../include/flashvm/rdispatch.h#L121) | Session-scoped queued cancellation; does not interrupt a callback. |
| [`fvm_rdispatch_take`](../include/flashvm/rdispatch.h#L126) | Copy and acknowledge completed result; releases identity and queue capacity. |
| [`fvm_rdispatch_destroy`](../include/flashvm/rdispatch.h#L131) | Quiesced owner frees only dispatcher when no accepted requests remain; nulls caller pointer. |


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
cmake --build build-resource --target flashvm_srecord flashvm_rdispatch_contract --parallel
ctest --test-dir build-resource -R '^flashvm_rdispatch_' --output-on-failure
```

SDL/network SDK не требуется. Cross задаёт target toolchain/libc; тесты запускаются
на target. Native OS/device/VM/GUI integration и Inferno/9P conformance открыты.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_rdispatch_cancel` | function | [include/flashvm/rdispatch.h:121](../include/flashvm/rdispatch.h#L121) |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:95](../include/flashvm/rdispatch.h#L95) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:131](../include/flashvm/rdispatch.h#L131) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:108](../include/flashvm/rdispatch.h#L108) |
| `fvm_rdispatch_lookup` | function | [include/flashvm/rdispatch.h:111](../include/flashvm/rdispatch.h#L111) |
| `fvm_rdispatch_post` | function | [include/flashvm/rdispatch.h:115](../include/flashvm/rdispatch.h#L115) |
| `fvm_rdispatch_pump` | function | [include/flashvm/rdispatch.h:119](../include/flashvm/rdispatch.h#L119) |
| `fvm_rdispatch_register` | function | [include/flashvm/rdispatch.h:99](../include/flashvm/rdispatch.h#L99) |
| `fvm_rdispatch_take` | function | [include/flashvm/rdispatch.h:126](../include/flashvm/rdispatch.h#L126) |
| `fvm_rdispatch_unregister` | function | [include/flashvm/rdispatch.h:105](../include/flashvm/rdispatch.h#L105) |
<!-- source-links:end -->
