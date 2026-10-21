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
flashvm_resource_frame_oracle. Oracle использует position-independent standalone
протокольный архив и не запускает VM/устройства.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_scontrol_active` | function | [include/flashvm/scontrol.h:108](../include/flashvm/scontrol.h#L108) |
<!-- source-links:end -->
