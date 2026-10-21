# Кадры ресурсов с удержанием

Аддитивный C89 API [frontend/common/resource_frame.h](../frontend/common/resource_frame.h) экспортирует копии RGBA
логической сцены через dispatcher, S-record и файлы ресурсов. Дополняет
[состояние таймлайна](PLAYER_RESOURCE_STATE.ru.md),
[захват кадров](FRAME_CAPTURE.ru.md), [9P](RESOURCE_9P.ru.md) и
[пример](../examples/player/frame_capture/README.ru.md). [EN](PLAYER_RESOURCE_FRAMES.md).

## Владение и лимиты

При create хост явно выбирает capacity одного буфера от 1 до
[FVM_DESKTOP_RFRAME_MAX_BYTES](../frontend/common/resource_frame.h#L28) (64 MiB). Буферов [FVM_DESKTOP_RFRAME_SLOTS](../frontend/common/resource_frame.h#L17) (три):
пиксельная память равна трём capacity плюс fixed native metadata. Все allocation
предшествуют регистрации. Ошибки освобождают частичные выделения, сохраняют оба
выхода. Неявного выделения максимального объёма нет; после create новые
allocation не выполняются ни при capture, ни при запросах/очистке.

Owner frame callback копирует видимые RGBA строки в spare, который не latest
и не удерживается. Padding источника отбрасывается. Сначала публикуется metadata,
затем новый latest. Указатели runner pixels/source/context/root не сохраняются.
Ошибка geometry/publish оставляет прежнее опубликованное изображение. Cache и
leases могут жить после runner/SDK teardown, пока жив dispatcher и вызовы owner
последовательны. [GC:] Managed roots, VM/SDL вызовов и collection здесь нет.

Успешный capture имеет ненулевой Uint32 sequence в жизни адаптера; исчерпание
LIMIT без оборачивания. Перед CHUNK клиент делает ACQUIRE. Изображение неизменно
до RELEASE, revoke или destroy, даже при поступлении новых кадров. INFO/events
сами pin не создают. Есть [FVM_DESKTOP_RFRAME_LEASES](../frontend/common/resource_frame.h#L19) (восемь) leases с отдельными
необорачивающимися поколениями и аутентифицированным владельцем-сессией.
Fairness, неявный timeout и идентичность после restart не обещаны. Читатель может
занять ограниченную квоту; потерянные/нежелательные сессии освобождает хост.

Если старые pins не оставили spare, capture даёт BUSY: прежние кадр/leases/
sequence сохраняются, dropped насыщаемо увеличивается. Notify-wrapper считает
BUSY успешным playback и пропускает только экспорт. INFO/ACQUIRE сообщают текущий
dropped; events — его значение на момент публикации. Uint32 max означает нижнюю
границу. Это отдельно от event.missed — потерь конкретной observer queue.
Capacity/sequence LIMIT не превращается в тихий пропуск. Capture не ждёт клиента
и не подтверждает доставку кадра по сети.

## Операции ресурса

Хост задаёт путь, например `/display/frame`. Текущая FS проецирует регистрацию
в каталог поколения: `/3.1/path` сообщает исходное имя, `/3.1/ops/2` вызывает
ACQUIRE. Настоящие slot/generation нужно перечислять, не угадывать. Все пять
операций требуют READ; SUBSCRIBE независимо разрешает metadata events. Знание
lease ID не даёт прав; wire uname/session claim не выбирает native authority.

| Операция | Параметры и результат |
| --- | --- |
| [FVM_DESKTOP_RFRAME_INFO](../frontend/common/resource_frame.h#L7) | Пустой request, metadata latest с lease zero/zero. До первого capture terminal BUSY. Pin не создаётся. |
| [FVM_DESKTOP_RFRAME_ACQUIRE](../frontend/common/resource_frame.h#L8) | Пустой request, metadata latest с новым lease ID/generation этой сессии. Заполненная/retired таблица LIMIT, ошибки pin не создают. Повторы намеренно создают независимые leases. |
| [FVM_DESKTOP_RFRAME_CHUNK](../frontend/common/resource_frame.h#L10) | Ровно четыре канонических неотрицательных S-целых: lease ID, generation, byte offset, count. Count 1..[FVM_R_BYTES](../include/flashvm/rdispatch.h#L9), offset не больше byte_count. До count сырых packed RGBA bytes; точный конец OK/0, дальше или неверный count INVALID. Живой чужой lease DENIED, старый/отозванный/неизвестный UNKNOWN. Выравнивание на пиксель не требуется. |
| [FVM_DESKTOP_RFRAME_RELEASE](../frontend/common/resource_frame.h#L13) | Ровно lease ID и generation как два S-целых. Освобождает только собственный pin; чужой DENIED, старый UNKNOWN. Успех — пустой ответ. При исчерпании generation слот окончательно retired. |
| [FVM_DESKTOP_RFRAME_REGION](../frontend/common/resource_frame.h#L29) | READ: lease ID, generation, x, y, width, height. Прямоугольник packed RGBA до 512 байт; неверная геометрия INVALID, большой ответ LIMIT, старый lease UNKNOWN, чужая сессия DENIED. |

Обычная resource write лишь ставит операцию в очередь, исполняет native owner
pump. Эти пять handler используют только native cache и допустимы после
возврата runner, без мёртвой VM. Во время playback — только безопасные owner
границы; control операции сохраняют отдельный notify-only контракт. Нельзя
pump-ить произвольные VM handlers из frame callback лишь потому, что конкретные
frame handlers обходятся без VM.

CHUNK reply — counted bytes в существующем checked resource reply. Metadata:
семнадцать канонических unsigned S-целых, затем source_size байт: version,
lease ID, lease generation, sequence, format, width, height, row_bytes, byte_count,
movie_ticks, total_ticks, presentations, total_presentations, reason, flags,
dropped, source_size. Строки width*4, сверху вниз, straight-alpha R/G/B/A octets.
Native endian pixel words/structs не передаются. Source побайтово усечён до 255
с явным флагом; UTF-8 code point может быть разрезан. Native counters больше
Uint32 насыщаются с флагом. Это host ticks/draws, не script `_currentframe`.
Кадры сырые, без видеокодека и синхронизации audio/video clocks.

## C API и время жизни

Изменения и handlers — только serialized owner. Worker post/take копии через
mutex policy диспетчера или декодирует свои metadata bytes. Входы/выходы не
перекрывают opaque storage и друг друга. Authentication, transport, установка
callback и выдача прав принадлежат хосту.

| Функция | Контракт |
| --- | --- |
| [fvm_desktop_resource_frame_create](../frontend/common/resource_frame.h#L69) | Dispatcher, canonical path, per-slot capacity, выходы adapter/target. Fixed native cache и пять READ операций, заимствование dispatcher до destroy. Некорректные аргументы/capacity INVALID, слишком большой объём LIMIT, allocation NOMEM, ошибки регистрации передаются. Выходы только на OK; SDK/device/grant не запускает. |
| [fvm_desktop_resource_frame_capture](../frontend/common/resource_frame.h#L78) | Адаптер и borrowed frame. Проверяет format/reason/dimensions/counters/source/visible extents, копирует packed pixels в spare, публикует metadata, меняет latest. Без allocation. Неверный frame INVALID, capacity/sequence LIMIT; нет spare BUSY с ростом только dropped. Ошибки publish сохраняют прежний опубликованный кадр. Политику ошибок/пропуска выбирает хост. |
| [fvm_desktop_resource_frame_notify](../frontend/common/resource_frame.h#L84) | Стандартный frame observer callback, userdata=adapter. OK/BUSY даёт [FVM_OK](../include/flashvm/flashvm.h#L42), при backpressure пропущен только экспорт. INVALID отображается в [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45), прочее — [FVM_ERROR](../include/flashvm/flashvm.h#L43). Не удерживает source/pixel/VM указателей. Можно вместо него явно вызывать capture со своей политикой. |
| [fvm_desktop_resource_frame_read](../frontend/common/resource_frame.h#L88) | Декодирует один counted metadata payload без outer envelope. Проверяет полную schema, lease pair/range, packed geometry, flags, ненулевые sequence/presentations, reason, source length и точный конец. INVALID сохраняет native output. Результат принадлежит клиенту, без adapter/VM и allocation. |
| [fvm_desktop_resource_frame_revoke](../frontend/common/resource_frame.h#L93) | Owner передаёт adapter и trusted session после остановки транспорта. Удаляет права ресурса, его watches и все pins сессии. Ошибка сохраняет pins; regrant старые поколения не возрождает. При полном disconnect также вызвать очистку сессии dispatcher. |
| [fvm_desktop_resource_frame_destroy](../frontend/common/resource_frame.h#L99) | Owner останавливает capture/handlers/clients, unregister точного поколения и free native buffers, инвалидируя даже открытые pins. Running handler BUSY сохраняет pointer. OK обнуляет pointer; queued old jobs затем UNKNOWN и требуют take/disconnect. Dispatcher/client files/SDK принадлежат хосту. |

Каждый lease освобождайте явно. Закрытие operation file, fid и даже 9P adapter
**не** освобождает специфичные pins автоматически. Потерянный ACQUIRE reply может
оставить pin до RELEASE/native revoke; повтор может создать ещё один. Disconnect
transport должен вызвать frame revoke до dispatcher disconnect либо destroy всего
frame adapter после quiesce. Простой внешний grant removal запретит дальнейшее
чтение, но pins сам не освободит. Владение не угадывается из недоверенных пакетов.

## Пример и сборка

`frame_capture --resource-frames` включает бюджет 4 MiB на слот (12 MiB пикселей),
экспорт всех представленных кадров и независимое чтение metadata events. Также
включается существующий resource-control/state режим. Обычный capture и
`--resource-control` сохраняют прежний бюджет. Большой кадр даёт явную ошибку.
Это локальный publisher/metadata reader, не listener и не вся remote GUI. PPM
по-прежнему отдельная owned копия, записывается после SDK teardown.
[Заголовок лимита](../examples/player/frame_capture/resource_limits.h).

```sh
cmake --build build-player --target flashvm_frontend_resource_frame flashvm_resource_frame_oracle flashvm_example_frame_capture
ctest --test-dir build-player -R '^flashvm_resource_frame' --output-on-failure
./build-player/frontend/flashvm_example_frame_capture --offscreen --resource-frames --embedded flashVM/examples/player/frame_capture/start.swf frame.ppm
```

Static SDL host настраивается по [руководству](DESKTOP_FRONTEND.ru.md). Standalone
contract: padded rows, старые неизменные leases, chunks/EOF, разные сессии,
release/revoke, лимиты buffers/leases, allocation failure на каждом выделении,
source/counter truncation, sequence/generation exhaustion и queued stale teardown.
Python integer oracle независимо проверяет metadata bytes, неверные geometry/
leases, все prefixes и native queue replies/chunks без старта VM/SDL. Tutorial
tests сравнивают каждый RGB пиксель для file, linked memory и настроенного HTTP.
Все стадии открыты; полного SWF/железа/OS mount/production transport и запуска
на чужой платформе эти проверки не доказывают.
## Пиксели через 9P v0.737

Live fixture [frame_capture.c](../frontend/tests/frame_capture.c) использует
реальный offscreen player и test-only C89 клиент
[resource_9p_client.c](../frontend/tests/resource_9p_client.c). Обе сессии 7/8
получают READ frame resource; session 7 отдельно имеет CONTROL таймлайна.
Каждая команда ACQUIRE/CHUNK/RELEASE/INFO передаётся стандартными 9P сообщениями
через файл операции, с существующими tag-zero reply bindings и CRC envelopes.
Нативные callbacks клиента не получают pixels/root/context.

При каждой публикации каждый клиент сначала перечитывает старый удержанный
кадр восемью CHUNK запросами по 512 байт и сравнивает все 4096 RGBA байт с прежней
owned копией. Затем RELEASE, ACQUIRE нового кадра, проверка metadata и полное
сравнение новых пикселей. Два lease ID различаются. Старое изображение остаётся
неизменным при смене кадров и навигации root; последовательность экспортов
соответствует presented frames, включая paused redraw. Отдельный независимый
SWF/pixel oracle проверяет фактические цвета/положение геометрии.

После runner return оба клиента снова читают удержанные финальные изображения
и INFO без доступа к уже уничтоженному контексту. После закрытия 9P/FS native
хост явно вызывает revoke для обеих сессий, затем destroy frame adapter.
Выполняются существующие FILE/MEMORY/HTTP сценарии, navigation и STOP/callback
failure при активном loadVariables с cancel/join. JIT сценарии требуют ненулевого
native counter, а не просто включённого флага JIT; отсутствующий backend SKIP.

```sh
cmake --build build-player --target flashvm_frontend_resource_9p_player
ctest --test-dir build-player -R '^flashvm_resource_9p_player_' --output-on-failure
```

Транспорт в этом player fixture внутренний; это проверка файлов/сообщений/
владения и реальных pixels, не TCP/UDP listener, auth service или ОС mount.
Предыдущий отдельный wire fixture 9P проверяет fragmented TCP loopback.
## Проверка серии v0.736–v0.737

На Linux x86-64 прошли 48 выбранных native и 13 shared-host тестов без SKIP.
Native сборка включала socket HTTP; shared — без HTTP providers с явной проверкой
отсутствующей возможности. Применялись offscreen/dummy, не физические устройства.
ASAN/UBSAN прошли allocation/lifecycle contract и новые frame/state/protocol/client
модули в реальном player fixture для interpreter и JIT. Основные VM/SDL архивы
в этом fixture обычные, leak enumeration отключён; дополнительно unit считает
все native выделения/освобождения при отказах и duplicate registration.
Это не полная санитайзерная приёмка VM.

C89 frame/lease contract кросс-слинкован для ARM64 Linux musl, RISC-V64 Linux musl
и Windows x64 GNU; C89 9P клиент и C++98 player-host boundary скомпилированы в
целевые объекты. Чужие бинарники не запускались. Самостоятельная проверка модуля:

```sh
zig cc -target aarch64-linux-musl -std=c89 -pedantic-errors \
  -IflashVM/include flashVM/frontend/tests/resource_frame.c \
  flashVM/frontend/common/frame.c flashVM/src/protocol/rdispatch.c \
  flashVM/src/protocol/srecord.c -o frame-contract-arm64
```

Fixture включает настоящий resource_frame.c с fault injection; VM/SDL ему не
нужны. Для всего player host требуются toolchain и static SDL SDK той же целевой
OS/CPU: см. [desktop build](DESKTOP_FRONTEND.ru.md) и
[платформенную приёмку](PLATFORM_VALIDATION.ru.md). Кросс-компиляция этого модуля
не заменяет сборку/запуск всего плеера, Android Activity/surface и проверку устройств.
## Области кадра v0.738

`FVM_DESKTOP_RFRAME_REGION` (операция 5, READ) принимает ровно шесть
канонических неотрицательных S-целых: lease ID, generation, x, y, width, height.
Координаты — логические пиксели сцены от левого верхнего угла, начиная с нуля.
Ширина и высота положительны; весь прямоугольник должен входить в закреплённый
кадр. Ответ — строки RGBA подряд, без padding и дополнительного заголовка;
размер ровно width*height*4 байт.

Ошибочная геометрия, лишние/недостающие поля или неканонические целые дают
INVALID. Допустимый прямоугольник больше [FVM_R_BYTES](../include/flashvm/rdispatch.h#L9) (512 байт) даёт LIMIT без
частичного результата: крупную область клиент разбивает на плитки. Старый lease
даёт UNKNOWN, чужой — DENIED. Чтение не выделяет память, не освобождает lease и
не изменяет кадр. Автоматическое вычисление изменившихся областей не добавлено.

Например, `[lease, generation, 7, 5, 16, 8]` возвращает 512 байт: восемь строк по
16 пикселей. Каждая строка ответа занимает 64 байта, stride полного кадра в ней
не сохраняется. Независимый Python oracle проверяет все области кадра 2x2 и
300 воспроизводимых областей padded-источника 8x32, включая предел 512 байт.
Два клиента 9P сравнивают внутреннюю область 16x8 настоящих кадров плеера,
старых закреплённых изображений и последних изображений после завершения VM.

В первом обмене Tversion/Rversion согласуется `msize` всего сообщения 9P,
включая заголовки. Отдельный предел [FVM_R_BYTES](../include/flashvm/rdispatch.h#L9) относится к payload
диспетчера. Если S-запись ответа больше согласованного msize, файл ответа
читается несколькими Tread/Rread. Увеличение msize не увеличивает внутренний
предел payload. См. [адаптер 9P](RESOURCE_9P.ru.md).
## Согласованный msize v0.739

Теперь клиенты live-плеера согласуют разные msize: 256 и 384 через
Tversion/Rversion до attach. Проверяется предел каждого полного ответа 9P,
включая заголовок. Payload 512 байт вместе с S-записью и оболочкой ресурса
требует нескольких чтений при обоих размерах. Клиент собирает ограниченный
по объёму файл ответа до EOF и затем разбирает полную S-запись с проверкой
checksum. Частичная запись не интерпретируется.

Так проверяются CHUNK и REGION настоящих кадров при небольших сообщениях 9P,
включая старые leases во время новых кадров и чтение после завершения VM.
Небольшие события таймлайна этого fixture помещаются в один Rread; клиент
не является универсальным сборщиком потока событий или сетевым сервером.
[FVM_R_BYTES](../include/flashvm/rdispatch.h#L9) остаётся 512, максимальный серверный msize — 4096; layout
публичных структур и версия метаданных не изменены.
Проверка v0.738–v0.739: прошли 48 выбранных native-тестов и 13 shared-host.
Новый код областей и live-fixture 9P прошли AddressSanitizer/UBSan; перечисление
утечек отключено из-за ограничений хоста. C89-контракт слинкован, клиент
скомпилирован для ARM64 Linux, RISC-V64 Linux и Windows x64. Чужие бинарники
не запускались; это не приёмка полного плеера на этих платформах.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_R_BYTES` | macro / define | [include/flashvm/rdispatch.h:9](../include/flashvm/rdispatch.h#L9) |
<!-- source-links:end -->
