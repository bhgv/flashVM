# Согласованные части настоящих кадров плеера

[English](PLAYER_FRAME_PARTS.md). C89-мост соединяет [ресурс кадров](PLAYER_RESOURCE_FRAMES.ru.md)
и [служебный/bulk-протокол](MEDIA_SLOTS.ru.md). Он экспортирует закреплённые
RGBA-байты с текущими правами сессии и committed boundary-aware соглашением.
Новых зависимостей от CLI и скрытых ресурсных wire-операций нет.

## API и владение

[`fvm_desktop_resource_frame_write_part`](../frontend/common/resource_frame.h#L110) принимает adapter, доверенную session,
lease, exchange, режим FILE/LIVE, относительный Uint32 time_ms, номер части,
буфер/ёмкость/used. Сначала проверяются текущие READ-права ресурса и точные
поколение/владелец lease. Одного номера недостаточно. Все вызовы сериализует
владелец cache; нельзя одновременно capture/revoke/destroy.

Соглашение должно содержать ненулевые stream/epoch/first-object. Object ID
части равен нециклическому capture sequence кадра; stream/epoch/tag берутся из
соглашения, total — точный packed RGBA extent. Offset и последняя длина зависят
от agreed part_bytes; каждый слот занимает agreed slot_bytes. Это байты R/G/B/A,
не native pixel words, не сжатая картинка и не видеокодек.

[`fvm_scontrol_active`](../include/flashvm/scontrol.h#L108) копирует проверенные committed limits/boundary. Pending
не меняет exporter. До первого соглашения UNKNOWN, ошибка сохраняет выходы.
Adapter не обязан читать изменяемые внутренние поля exchange. Exchange остаётся
собственностью хоста, без транспорта и неявной аутентификации.

Заголовок и диапазон неизменяемых пикселей кодируются scatter-вводом: нет второй
копии всего кадра, выделений, VM/SDL, callback, pump или socket/file I/O. Output
принадлежит клиенту и не перекрывает input/state; ошибки сохраняют output/used.
Capture уже скопировал кадр, поэтому экспорт допустим после runner/SDK teardown
при живых lease/adapter/dispatcher. Кодирование не означает доставку и не
освобождает pin. Release/revoke выполняются явно после окончания повторов.

| Результат | Значение |
| --- | --- |
| OK | Записана одна полная padded-часть. |
| DENIED | Нет текущих READ-прав или lease другой сессии. |
| UNKNOWN | Старый/освобождённый/неизвестный lease либо исчезнувший ресурс. |
| BUSY | Committed соглашения пока нет; одного pending недостаточно. |
| CONFLICT | Кадр раньше first-object; для старых кадров нужно сохранить старое соглашение. |
| LIMIT | Малая ёмкость, кадр больше квоты объекта или слишком много частей. |
| INVALID | Неверные native arguments, compact/non-boundary соглашение, mode/time/part. |

Текущая квота объекта — 3 МиБ и до 4096 частей. Более крупные кадры остаются
доступны через CHUNK/REGION, но не обрезаются и не масштабируются этим API.
Разделение на несколько объектов требует отдельного явного контейнера.
Размеры/источник/ticks остаются в ACQUIRE metadata и должны храниться вместе с
identity: pixel part сама по себе не задаёт геометрию и color space.

## Переключение профиля и транспорт

Для нового capture sequence согласуйте OFFER_AT/ACCEPT_AT с этим first_object.
Старые agreement/lease сохраняются до окончания повторов старого кадра. Новый
размер не переинтерпретирует его буфер. Новый adapter или повторно используемый
stream требует новой epoch хоста. Изменение размера не меняет таймлайн/renderer.

Слоты — собственные байты клиента. Их можно объединять для надёжного файла/TCP
либо передавать по подходящему MTU через paced/authenticated UDP. Msize, MTU и
record quotas различаются. Существующая квота operation-файла остаётся 512 байт;
это явный native bulk API, а не скрыто увеличенный ответ 9P. Автоматической
адаптации, listener, socket queue, видеокодека, A/V sync и удалённого окна нет.

## Проверка и сборка

C-contract использует padded-строки и изменение исходных пикселей после capture;
проверяет точные байты, отсутствие новых выделений, малую ёмкость, pending,
чужую сессию, stale pin и будущую границу. Независимый Python integer/zlib oracle
строит ожидаемый слот 256 байт и проверяет атомарность ошибок.

Сборка host с подходящим static SDL SDK: [desktop](DESKTOP_FRONTEND.md).
Цели: flashvm_player_host, flashvm_frontend_resource_frame,
flashvm_resource_frame_oracle. Oracle собирает отдельные протокольные C-модули напрямую для экспорта C-символов
из DLL/shared object и не запускает VM/устройства.
## Настоящий плеер и транспорт v0.744

[Прокомментированный frame_parts host](../examples/player/frame_parts/README.ru.md)
исполняет три настоящих linked-SWF кадра offscreen и пишет компактные привязки,
metadata и OFFER_AT/ACCEPT_AT до bulk-слотов. Проверяются 1024/768 -> 512/400 ->
4096/4000 и повторный экспорт закреплённого слота после SDK shutdown. Режимы
FILE/LIVE выбираются явно; библиотека не зависит от file adapter и CLI.

Независимый integer/zlib Python reader сверяет wire bytes, identity, padding и
каждый RGBA byte с authored SWF-квадратами. Затем настоящие экспортированные
слоты проходят IPv4 UDP loopback с перестановкой, дубликатами, разрушенной и
пропущенной частями. Native receiver перечисляет FILE-пропуски; только они
запрашиваются и повторяются по TCP. LIVE не запрашивает повторы. Сокеты принадлежат
тесту, не production listener и не автоматической path-MTU политике.

Два live 9P-reader fixture также экспортируют собственные разрешённые leases через
native bulk API. Раздельные направления уменьшают/увеличивают профиль между
кадрами. Старые кадры собираются при уже опубликованных новых, последние — после
runner teardown. Сохраняются проверки настоящих interpreter/native-JIT, local
file/linked-memory/HTTP navigation, callback failure и STOP cancellation. Msize 9P
независимо остаются 256/384: большие bulk-слоты не маскируются под operation reply.

Прошли 54 выбранных native проверки: одному старому executable потребовалось
восстановить потерянный хостом execute-bit, после чего он прошёл. Прошли все 14
shared-host проверок. Native frame contract, изменённые protocol/cache/client
модули с настоящими interpreter/JIT player и полный пример прошли ASan/UBSan.
Leak enumeration отключено ограничениями хоста; вся VM/SDL не инструментирована,
поэтому приёмка memory safety всей программы не заявляется.

C89 frame contract слинкован, tutorial C89/public-header C++98 consumers
скомпилированы для ARM64 Linux, RISC-V64 Linux и Windows x64. Иностранные
бинарники не запускались; полный иностранный плеер не линковался. Из корня:

```sh
zig cc -target aarch64-linux-musl -std=c89 -pedantic-errors -IflashVM/include \
  flashVM/frontend/tests/resource_frame.c flashVM/frontend/common/frame.c \
  flashVM/src/protocol/srecord.c flashVM/src/protocol/rdispatch.c \
  flashVM/src/protocol/sslot.c flashVM/src/protocol/scontrol.c \
  flashVM/src/protocol/smedia.c -o frame-parts-contract-arm64
```

Остальные проверенные target: riscv64-linux-musl и x86_64-windows-gnu. Полный host
требует target static SDL SDK/адаптер: [платформы](BUILD_PLATFORMS.ru.md). Приёмка
native devices/платформ и все девять стадий остаются открытыми.
Дополнительно для Windows x64 слинкована standalone C89 oracle DLL; export table
содержит новый writer, committed-profile getter и native receiver API. DLL не
запускалась. Исходники проверок: [native contract](../frontend/tests/resource_frame.c),
[независимый reader](../frontend/tests/frame_parts_example.py),
[live player fixture](../frontend/tests/frame_capture.c).

See also / См. также: [v0.746 feedback-driven replay](CHANNEL_ADAPTATION.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_scontrol_active` | function | [include/flashvm/scontrol.h:108](../include/flashvm/scontrol.h#L108) |
<!-- source-links:end -->
