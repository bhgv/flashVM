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
ACQUIRE. Настоящие slot/generation нужно перечислять, не угадывать. Все четыре
операции требуют READ; SUBSCRIBE независимо разрешает metadata events. Знание
lease ID не даёт прав; wire uname/session claim не выбирает native authority.

| Операция | Параметры и результат |
| --- | --- |
| [FVM_DESKTOP_RFRAME_INFO](../frontend/common/resource_frame.h#L7) | Пустой request, metadata latest с lease zero/zero. До первого capture terminal BUSY. Pin не создаётся. |
| [FVM_DESKTOP_RFRAME_ACQUIRE](../frontend/common/resource_frame.h#L8) | Пустой request, metadata latest с новым lease ID/generation этой сессии. Заполненная/retired таблица LIMIT, ошибки pin не создают. Повторы намеренно создают независимые leases. |
| [FVM_DESKTOP_RFRAME_CHUNK](../frontend/common/resource_frame.h#L10) | Ровно четыре канонических неотрицательных S-целых: lease ID, generation, byte offset, count. Count 1..[FVM_R_BYTES](../include/flashvm/rdispatch.h#L9), offset не больше byte_count. До count сырых packed RGBA bytes; точный конец OK/0, дальше или неверный count INVALID. Живой чужой lease DENIED, старый/отозванный/неизвестный UNKNOWN. Выравнивание на пиксель не требуется. |
| [FVM_DESKTOP_RFRAME_RELEASE](../frontend/common/resource_frame.h#L13) | Ровно lease ID и generation как два S-целых. Освобождает только собственный pin; чужой DENIED, старый UNKNOWN. Успех — пустой ответ. При исчерпании generation слот окончательно retired. |

Обычная resource write лишь ставит операцию в очередь, исполняет native owner
pump. Эти четыре handler используют только native cache и допустимы после
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
| [fvm_desktop_resource_frame_create](../frontend/common/resource_frame.h#L67) | Dispatcher, canonical path, per-slot capacity, выходы adapter/target. Fixed native cache и четыре READ операции, заимствование dispatcher до destroy. Некорректные аргументы/capacity INVALID, слишком большой объём LIMIT, allocation NOMEM, ошибки регистрации передаются. Выходы только на OK; SDK/device/grant не запускает. |
| [fvm_desktop_resource_frame_capture](../frontend/common/resource_frame.h#L76) | Адаптер и borrowed frame. Проверяет format/reason/dimensions/counters/source/visible extents, копирует packed pixels в spare, публикует metadata, меняет latest. Без allocation. Неверный frame INVALID, capacity/sequence LIMIT; нет spare BUSY с ростом только dropped. Ошибки publish сохраняют прежний опубликованный кадр. Политику ошибок/пропуска выбирает хост. |
| [fvm_desktop_resource_frame_notify](../frontend/common/resource_frame.h#L82) | Стандартный frame observer callback, userdata=adapter. OK/BUSY даёт [FVM_OK](../include/flashvm/flashvm.h#L42), при backpressure пропущен только экспорт. INVALID отображается в [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45), прочее — [FVM_ERROR](../include/flashvm/flashvm.h#L43). Не удерживает source/pixel/VM указателей. Можно вместо него явно вызывать capture со своей политикой. |
| [fvm_desktop_resource_frame_read](../frontend/common/resource_frame.h#L86) | Декодирует один counted metadata payload без outer envelope. Проверяет полную schema, lease pair/range, packed geometry, flags, ненулевые sequence/presentations, reason, source length и точный конец. INVALID сохраняет native output. Результат принадлежит клиенту, без adapter/VM и allocation. |
| [fvm_desktop_resource_frame_revoke](../frontend/common/resource_frame.h#L91) | Owner передаёт adapter и trusted session после остановки транспорта. Удаляет права ресурса, его watches и все pins сессии. Ошибка сохраняет pins; regrant старые поколения не возрождает. При полном disconnect также вызвать очистку сессии dispatcher. |
| [fvm_desktop_resource_frame_destroy](../frontend/common/resource_frame.h#L97) | Owner останавливает capture/handlers/clients, unregister точного поколения и free native buffers, инвалидируя даже открытые pins. Running handler BUSY сохраняет pointer. OK обнуляет pointer; queued old jobs затем UNKNOWN и требуют take/disconnect. Dispatcher/client files/SDK принадлежат хосту. |

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
