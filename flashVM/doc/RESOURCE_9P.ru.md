# Адаптер ресурсов 9P2000

`r9p.h` — собственный ограниченный C89 обработчик сообщений поверх
[файлов ресурсов](RESOURCE_FILES.ru.md), включённый в flashvm_srecord и SDK.
Хост предоставляет аутентифицированную сессию, надёжный упорядоченный транспорт,
накопление кадров, пробуждения и owner pump. Сокет listener, OS mount, шифрование,
UDP и пространство имён Inferno автоматически не создаются.
[Пример](../examples/embedding/resource_9p/README.ru.md),
[отладчик](RESOURCE_DEBUGGER.ru.md), [EN](RESOURCE_9P.md).

## Формат и операции

Внешний кадр, строки, qid и stat используют стандартный **little-endian 9P2000**.
Содержимое файлов операций/событий сохраняет отдельный **S-формат**: компактные
big-endian целые, tag-zero объявления и CRC. Нельзя кодировать поля S-записи
правилами внешнего 9P кадра. Это подмножество device-server, не полная дисковая
FS и не все возможности Inferno.

| Запрос | Поведение |
| --- | --- |
| Tversion | NOTAG, msize не меньше [FVM_R9P_MIN_MSIZE](../include/flashvm/r9p.h#L6); верх ограничен [FVM_R9P_MSIZE](../include/flashvm/r9p.h#L5). 9P2000 и суффикс через точку дают 9P2000; неизвестная версия — `unknown`, после чего нужно новое согласование. Reset закрывает fids, отменяет queued file calls и сбрасывает pending reads. Owner callbacks перед reset должны закончиться; при BUSY часть fids уже может быть закрыта, read tags сброшены. |
| Tauth / Tattach | Tauth даёт Rerror: аутентификацией занимается транспорт хоста. Attach принимает только NOFID afid и пустой aname. uname информативен и не выдаёт прав. Корень ограничен сессией FS. Повторный fid и NOFID запрещены; fid ноль допустим. |
| Twalk | До [FVM_R9P_WELEM](../include/flashvm/r9p.h#L8) компонентов. Ноль компонентов клонирует неоткрытый fid; поддерживается `..`. Частичный walk возвращает успешные qid, но не меняет newfid; первая ошибка даёт Rerror. Открытые fids не обходятся; существующий другой newfid конфликтует. |
| Topen | OREAD для каталогов/path/events, ORDWR для операций. Write-only, execute, truncate, remove-on-close и другие flags отвергаются. Iounit — минимум [FVM_R_BYTES](../include/flashvm/rdispatch.h#L9) и msize минус стандартный резерв 24 байта. Один атомарный write на open операции; следующий вызов требует reopen. |
| Tread | Каталог содержит целые stat-записи. Offset ноль либо предыдущий offset плюс число возвращённых байт. Ноль перематывает каталог; малый ненулевой count для следующей записи даёт Rerror, а не ложный EOF. Изменение discovery требует reopen. PATH допускает 64-битное смещение: ненулевая старшая половина даёт EOF. CALL/EVENTS — последовательные Uint32 потоки; пока результата нет, read удерживается. Один pending read на fid; count ноль немедленно даёт пустой ответ. |
| Twrite | Один counted raw payload, offset ноль, не больше [FVM_R_BYTES](../include/flashvm/rdispatch.h#L9) и negotiated frame. Rwrite подтверждает очередь, а не исполнение; callback вызывается отдельным owner pump. Дробление команды на несколько Twrite не поддержано. |
| Tflush | Подавляет ещё не произведённый ответ pending read по oldtag; Rflush также для неизвестного/завершённого тега. Не закрывает подписку, не изымает событие, не отменяет предыдущий Twrite и не откатывает эффекты. Повреждённый flush отвергается native framing boundary, без Rerror. |
| Tclunk | Делает wire fid недействительным даже при ошибке close. Его pending read получает отложенный Rerror, если не flushed; новый fid не подменит этот read. Исполняемый callback может оставить retired native slot до последующего poll/destroy. Исполненные эффекты не откатываются. |
| Tstat | Внутренний stat size плюс внешний counted size. Qid path использует поколение ресурса; content version, type/dev, времена и длина равны нулю для синтетических device files. Owner/group/muid `host` информативны; mode отражает допустимое открытие, но native права сессии остаются решающими. |
| Остальное | Create, remove, wstat, auth exchange и расширения протокола дают Rerror. Регистрации и файлы создаёт доверенный C API, не произвольные клиентские имена. |

Device повторно проверяет native права даже у открытых файлов, включая остаток
события в буфере; отозванное право не живёт до clunk. Revoke/regrant SUBSCRIBE
не восстанавливает старый watch. Закрытие соединения не удаляет общие ресурсы
и не отключает посторонние сессии.

## C API и владение

Хост последовательно вызывает API. Сервер заимствует один исключительный FS;
пока его дескрипторы принадлежат адаптеру, FS нельзя независимо использовать
или удалять. Управляемых указателей/GC roots, потоков, часов и callbacks нет.
Исполнение остаётся в [fvm_rdispatch_pump](../include/flashvm/rdispatch.h#L119) или owner service отладчика.
Выходной буфер всегда [FVM_R9P_MSIZE](../include/flashvm/r9p.h#L5), даже при меньшем negotiated msize.
Вход/выход/счётчики не перекрываются. Все произведённые ответы хост удерживает
и отправляет по порядку, включая ответ перед последующим Rflush; API не
подтверждает доставку произведённых байт.

| Функция | Контракт |
| --- | --- |
| [`fvm_r9p_create`](../include/flashvm/r9p.h#L20) | Заимствует FS, выделяет один fixed native server. Выход меняется только на OK; NOMEM сохраняет его. FS уже содержит доверенную сессию и права. Один FS на соединение; сокетов/mount/auth не запускает. |
| [`fvm_r9p_request`](../include/flashvm/r9p.h#L28) | Сервер, input/count, output/capacity, used/consumed. Обрабатывает ровно один кадр, оставляя следующий склеенный кадр непрочитанным. Малая ёмкость до MSIZE даёт LIMIT до изменений. Неполный ввод PENDING, неверный frame INVALID/LIMIT, повтор outstanding tag CONFLICT — хост прекращает нарушившее протокол соединение. Эти native ошибки сохраняют выходы. Валидный запрос даёт OK и consumed=длина кадра, used=ответ либо ноль для удержанного Tread. Ошибки сервиса/протокола — полные Rerror. VM не исполняет. |
| [`fvm_r9p_poll`](../include/flashvm/r9p.h#L34) | Сервер, MSIZE буфер, used. Round-robin проверяет reads, производит один ответ и освобождает tag. PENDING сохраняет выход/used. Также повторяет retired close после завершения callback. Ответы могут быть не в порядке запросов: сопоставлять по tag. Произведённые байты хост удерживает для partial-send retry. |
| [`fvm_r9p_destroy`](../include/flashvm/r9p.h#L39) | Указатель на сервер после остановки транспорта/owner callbacks. Закрывает fids и отбрасывает reads. BUSY сохраняет сервер, но часть fids уже может быть закрыта: повторить после callback. OK освобождает сервер и обнуляет указатель. Затем удалить FS, при необходимости отключить сессию на owner, а диспетчер — после окончания всех заимствований. |

Лимиты: [FVM_R9P_FIDS](../include/flashvm/r9p.h#L7) fids и pending reads, [FVM_R9P_WELEM](../include/flashvm/r9p.h#L8) компонентов walk,
[FVM_R9P_MSIZE](../include/flashvm/r9p.h#L5) на кадр. Retired running-close slot занят до очистки. Дополнительно
действуют лимиты FS/диспетчера. Выделение только при create. Offset разбит на две
Uint32 половины, без C99 long long и native endian layout. Хост ограничивает
receive/send queues и накапливает кадр перед request; транспортного backlog
в адаптере нет.

## Проверки и платформы

```sh
cmake -S flashVM -B build-9p -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON
cmake --build build-9p --target flashvm_srecord_oracle flashvm_r9p_debugger flashvm_r9p_lifetime flashvm_examples_embedding_resource_9p_main
ctest --test-dir build-9p -R '^flashvm_r9p_' --output-on-failure
```

`tests/resources/ninep.py` независимо строит 9P через Python struct и ожидаемые
S-записи через Python integer/zlib: partial/clone walk, stat/offset, атомарность,
512 байт, pending read, flush/clunk/reset, права, лимиты, неизвестные версии,
1500 случайных/повреждённых сообщений и настоящий IPv4 TCP loopback с фрагментами
по байту и склеенными кадрами. Транспорт предоставляет тестовый Python host;
это не production C listener и не смонтированная FS.
`ninep_debugger.c` исполняет настоящую pinned AVM1 функцию с результатом 42:
два 9P соединения получают STOP/RESUME, контроллер делает claim/resume, read-only
наблюдатель запрашивает interpreter frame. Проверяются source mapping и
маршрутизация ответов вне порядка запросов по tag. `ninep_lifetime.c` проверяет
отказ allocation и clunk внутри running callback с последующей очисткой.
Полную SWF/9P/Inferno совместимость эти проверки не доказывают.

C89 API доступен из C++98, новых зависимостей нет; применяются существующие
native/cross toolchains компонента. Кросс-компиляция protocol fixtures не заменяет
запуск на чужой OS, mount и проверку устройств. См.
[платформенную приёмку](PLATFORM_VALIDATION.ru.md). Первичные спецификации:
[intro](https://9fans.github.io/plan9port/man/man9/intro.html),
[walk](https://9fans.github.io/plan9port/man/man9/walk.html),
[read/write](https://9fans.github.io/plan9port/man/man9/read.html),
[stat](https://9fans.github.io/plan9port/man/man9/stat.html),
[flush](https://9fans.github.io/plan9port/man/man9/flush.html),
[clunk](https://9fans.github.io/plan9port/man/man9/clunk.html).
## Контрольная точка v0.733

На Linux x86-64 прошли 42 выбранных native теста, включая offscreen/file/memory/HTTP
плеер, и ещё 7 в shared-host конфигурации без HTTP. Новые C-модули, lifecycle,
пример и реальный AVM1/9P fixture проверены ASAN/UBSAN; основная VM в этом
санитайзерном fixture использует обычные архивы, leak enumeration отключён.
Это не полная санитайзерная проверка VM. C89 9P пример кросс-слинкован для
ARM64 Linux musl, RISC-V64 Linux musl и Windows x64 GNU; debugger fixture и
C++98 public ABI probe скомпилированы в target objects. Чужие бинарники не
запускались. SDL-проверки использовали dummy/offscreen, не физические устройства.

Для собственного toolchain из корня репозитория (замените абсолютный путь):

```sh
cmake -S flashVM -B build-cross-9p \
  -DCMAKE_TOOLCHAIN_FILE=/absolute/path/to/target-toolchain.cmake \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs" \
  -DFVM_BUILD_TESTS=OFF -DFVM_BUILD_EXAMPLES=ON
cmake --build build-cross-9p --target flashvm_srecord flashvm_examples_embedding_resource_9p_main
```

Toolchain выбирает OS, CPU, компилятор и sysroot; готовый target executable
переносится на целевую платформу для отдельного запуска. В отделённом компоненте
используйте `-S .` и `$PWD/cmake/profiles/minimal.defs`. Настройка NDK/MinGW и
нативная приёмка описаны в [платформенном руководстве](PLATFORM_VALIDATION.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_r9p_create` | function | [include/flashvm/r9p.h:20](../include/flashvm/r9p.h#L20) |
| `fvm_r9p_destroy` | function | [include/flashvm/r9p.h:39](../include/flashvm/r9p.h#L39) |
| `FVM_R9P_FIDS` | macro / define | [include/flashvm/r9p.h:7](../include/flashvm/r9p.h#L7) |
| `FVM_R9P_MIN_MSIZE` | macro / define | [include/flashvm/r9p.h:6](../include/flashvm/r9p.h#L6) |
| `FVM_R9P_MSIZE` | macro / define | [include/flashvm/r9p.h:5](../include/flashvm/r9p.h#L5) |
| `fvm_r9p_poll` | function | [include/flashvm/r9p.h:34](../include/flashvm/r9p.h#L34) |
| `fvm_r9p_request` | function | [include/flashvm/r9p.h:28](../include/flashvm/r9p.h#L28) |
| `FVM_R9P_WELEM` | macro / define | [include/flashvm/r9p.h:8](../include/flashvm/r9p.h#L8) |
| `FVM_R_BYTES` | macro / define | [include/flashvm/rdispatch.h:9](../include/flashvm/rdispatch.h#L9) |
| `fvm_rdispatch_pump` | function | [include/flashvm/rdispatch.h:119](../include/flashvm/rdispatch.h#L119) |
<!-- source-links:end -->
