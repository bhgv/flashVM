# Настоящие кадры в согласованные S-части

[English](README.md), [контракт](../../../doc/PLAYER_FRAME_PARTS.ru.md),
[каталог](../../README.md#example-player-frame_parts).
[main.c](main.c), [limits.h](limits.h). Общие [linked SWF bytes](../frame_capture/movie.inc)
генерируются [make_movie.py](../frame_capture/make_movie.py); это реальный SWF8,
не подмена renderer. Нечётные ticks рисуют синий квадрат слева, чётные — оранжевый
справа. Исполняются три кадра в настоящем offscreen-плеере.

Пример сначала передаёт компактную tag-zero привязку двух типов. Для каждого
кадра ACQUIRE даёт immutable lease и metadata; отдельный tag-zero обмен согласует
размеры 1024/768, 512/400, затем 4096/4000. Кодируются реальные packed RGBA части,
а не ссылки на callback-пиксели. Последний lease хранится до SDK shutdown;
повторное кодирование первой части должно дать те же байты. Ранние pins явно
освобождаются, cleanup отзывает оставшиеся.

Обычные сообщения переменной длины не получают padding. Bulk-режим включён
явно и может уменьшаться/увеличиваться только с согласованного нового кадра.
Опциональный replay обратной связи описан ниже; сетевые измерения собирает хост. FILE допускает запросы пропусков,
LIVE их не формирует. Файл сохраняет выбранный режим, но сам пример не открывает
сетевые сокеты; TCP/UDP-проверка выполняется независимым тестом.

```sh
cmake -S flashVM -B build-frame-parts -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DSDL2_DIR=/path/to/native-sdl/lib/cmake/SDL2
cmake --build build-frame-parts --target flashvm_example_frame_parts flashvm_resource_frame_oracle
./build-frame-parts/frontend/flashvm_example_frame_parts frames.sparts
./build-frame-parts/frontend/flashvm_example_frame_parts --live live.sparts
ctest --test-dir build-frame-parts -R flashvm_resource_frame_parts_example --output-on-failure
```

Нужны C89/C++98, CMake и target SDL2 SDK. SDK/VM, bundled graphics и зависимости
линкуются обычным host target; Python требуется только тесту. Видео/окно/аудио
не инициализируются: пример mute/offscreen. Из папки компонента используйте
`cmake -S .` и относительный путь к profile. `--help` не открывает файл и SDK.
Выход перезаписывает указанный файл; ошибка удаляет только открытый этой
инвокацией partial output. Ожидаемый stdout: `exported_frames=3`.

## Формат и владение

Это демонстрационный бинарный stream, не универсальный видеоконтейнер:
сначала tag 0 legacy bindings, затем для каждого кадра tag 20 metadata
(существующие 17 полей и source), компактные OFFER_AT/ACCEPT_AT, затем известное
число одинаковых по размеру слотов tag 19. Размер последней части точный,
padding слота нулевой. Metadata сохраняют геометрию; lease ID в файле не даёт
читателю права доступа. Времена sequence*40 — явно выбранные tutorial metadata,
не измерение FPS и не гарантия A/V synchronization.

Dispatcher частный: callback pump исполняет только native cache handlers, не
произвольный пользовательский код. Main владеет file/cache/exchanges; исходные
SWF bytes живут до runner return, кадр callback заимствуется только до capture.
На выход нужен один caller slot 4096 байт и три cache slot по 4096; второго
буфера всего изображения внутри exporter нет. Не передавайте переиспользуемый
output pointer worker: очередь должна владеть своими байтами и иметь backpressure.
API ничего не отправляет и не освобождает pin автоматически.

Тест проверяет каждый RGBA byte независимыми integer/zlib и SWF-пиксельными
ожиданиями, затем подаёт настоящие exported slots native receiver через IPv4 UDP
loopback с перестановкой, дубликатами, повреждением и пропуском. Для FILE список
пропусков передаётся компактно по TCP и повторяются только эти части; LIVE
повторов не делает. Loopback принимает выбранные размеры; приемлемость датаграмм
4096 байт для произвольного internet MTU не заявляется. Реального listener,
аутентификации, congestion control, видеокодека и удалённого окна пример не даёт.

| API | Ownership / выполнение |
| --- | --- |
| [`fvm_rdispatch_create`](../../../include/flashvm/rdispatch.h#L109) | Создаёт ограниченную частную очередь без потоков/VM; освобождается после adapter. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| `fvm_desktop_resource_frame_create` | Выделяет три слота по 4096 байт, регистрирует native handlers; ошибка чистит частичное владение, dispatcher заимствуется. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_rdispatch_grant`](../../../include/flashvm/rdispatch.h#L128) | Явно выдаёт READ доверенной сессии; wire metadata не создаёт права. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_scontrol_init`](../../../include/flashvm/scontrol.h#L86) | Копирует реальные пределы буфера без активации профиля и выделений. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_sbindings_payload_write`](../../../include/flashvm/sbindings.h#L51) | Кодирует две неизменяемые привязки типов направления; input заимствуется только на вызов. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_srecord_write`](../../../include/flashvm/srecord.h#L76) | Оборачивает metadata/привязки в компактные записи; output/used атомарны. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| `fvm_desktop_options_init` | Инициализирует options до выбора mute/числа кадров/interpreter. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| `fvm_desktop_frame_observer_init` | Инициализирует ABI observer; callback заимствует owner state до возврата runner. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| `fvm_desktop_run_source_offscreen` | Синхронно запускает linked SWF/raster без SDL video/events; байты живут до возврата. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| `fvm_desktop_resource_frame_capture` | Копирует пиксели в свободный cache slot; указатели не сохраняет, BUSY требует политики хоста. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_rdispatch_post`](../../../include/flashvm/rdispatch.h#L139) | Копирует запрос cache; принятый ID нужно take либо cancel/take. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_rdispatch_pump`](../../../include/flashvm/rdispatch.h#L145) | Выполняет одну native cache операцию на владельце; произвольные VM handlers здесь не исполняются. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_rdispatch_take`](../../../include/flashvm/rdispatch.h#L156) | Копирует terminal reply и освобождает слот запроса. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_rdispatch_cancel`](../../../include/flashvm/rdispatch.h#L149) | Отменяет queued job при ошибке pump; затем нужен take ответа отмены. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| `fvm_desktop_resource_frame_read` | Строго разбирает собственные ACQUIRE metadata с lease и геометрией. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_scontrol_propose_at`](../../../include/flashvm/scontrol.h#L123) | Предлагает границу capture sequence, прежний active не меняет. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_scontrol_write`](../../../include/flashvm/scontrol.h#L75) | Кодирует малый tag-zero offer/ACK без заполнения. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_scontrol_read`](../../../include/flashvm/scontrol.h#L79) | Проверяет service bytes, копирует одну запись; остаток не потребляет. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_scontrol_respond`](../../../include/flashvm/scontrol.h#L99) | Выбирает меньшие квоты в pending ACK. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_scontrol_commit`](../../../include/flashvm/scontrol.h#L108) | Активирует точный/коррелированный ACK после надёжной очереди, без сдвига границы. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_scontrol_active`](../../../include/flashvm/scontrol.h#L132) | Копирует выбранные committed limits; UNKNOWN до соглашения, ошибки сохраняют выходы. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| `fvm_desktop_resource_frame_write_part` | Проверяет READ/владельца lease, кодирует часть без выделений и VM. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_sinteger_write`](../../../include/flashvm/srecord.h#L62) | Кодирует два переносимых поля release, без native-endian структур. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| `fvm_desktop_resource_frame_revoke` | Владелец после quiescence снимает права/все pins, включая ошибки. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| `fvm_desktop_resource_frame_destroy` | Снимает регистрацию и освобождает cache после callback; успех обнуляет указатель. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| [`fvm_rdispatch_destroy`](../../../include/flashvm/rdispatch.h#L163) | Освобождает quiesced очередь после adapter/replies. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |
| `fvm_desktop_shutdown` | Проверяет завершение runner/SDL; последний native export выполняется после shutdown. [Contract](../../../doc/PLAYER_FRAME_PARTS.ru.md). |

## Настоящие кадры с обратной связью (v0.746)

[Запускаемый пример frame_parts](README.ru.md)
принимает `--feedback reports.srecords`: ровно три компактных отчёта, целиком
проверяемых до открытия результата. Идентификаторы: stream 7, epoch 1, window
1/2/3. При исполнении expected должен совпасть с числом частей предыдущего
кадра. Это проверяет причинную привязку, но не подлинность файла или измерений.

Четыре настоящих SWF кадра начинают с 1024/768. Независимый тест задаёт окно:
шесть ожидаемых частей, четыре вовремя, одна повреждена, одна отсутствует;
затем чистые окна из одиннадцати и шести частей, известный RTT 20 мс. Политика
выбирает 512/400, 1024/768, затем 4096/4000. Только для короткого примера
достаточно одного плохого/хорошего окна, минимум выборки один, cooldown ноль.
Это явные демонстрационные настройки, а не рекомендации для реальной сети.

Кандидат проходит компактные OFFER_AT/ACK на границе следующего изображения;
лишь согласованный профиль подтверждается политике и передаётся writer кадров.
Транскрипт содержит входящую обратную связь перед метаданными кадров 2–4:
он отражает оба направления, а не односторонний видеоконтейнер. Без опции
сохраняется прежний сценарий из трёх кадров с заданными размерами.

Независимый reader сравнивает все RGBA байты и поля, затем передаёт реальные
части через UDP loopback с потерями, повреждениями, повторами и перестановками.
FILE восстанавливает только недостающие части через TCP; LIVE не переспрашивает.
Проверяются испорченные, чрезмерные и лишние отчёты, чужие идентификаторы,
порядок окон, неверный expected и ошибки CLI. Ошибка чтения входа сохраняет
существующий результат; ошибка после открытия удаляет частичный файл этого запуска.

Это воспроизведение явных измерений хоста. Автоматический сетевой сборщик,
контроль перегрузки, аутентифицированный сервер и видеокодек пока не добавлены.

| API | Ownership / результат |
| --- | --- |
| [`fvm_sadapt_config_init`](../../../include/flashvm/sadapt.h#L104) | Заполняет примерные настройки до явных поправок демо; без выделения памяти. [Contract](../../../doc/CHANNEL_ADAPTATION.ru.md). |
| [`fvm_sadapt_init`](../../../include/flashvm/sadapt.h#L110) | Проверяет и копирует три профиля; состояние остаётся по неизменному адресу до завершения. [Contract](../../../doc/CHANNEL_ADAPTATION.ru.md). |
| [`fvm_sadapt_feedback_read`](../../../include/flashvm/sadapt.h#L139) | Проверяет компактный отчёт и копирует лишь при успехе; все три записи разбираются до открытия результата. [Contract](../../../doc/CHANNEL_ADAPTATION.ru.md). |
| [`fvm_sadapt_observe`](../../../include/flashvm/sadapt.h#L118) | Принимает окно предыдущего кадра и возвращает кандидата без изменения framing и сетевой отправки. [Contract](../../../doc/CHANNEL_ADAPTATION.ru.md). |
| [`fvm_sadapt_feedback_write`](../../../include/flashvm/sadapt.h#L135) | Записывает проверенный отчёт обратного направления в транскрипт; выходными байтами владеет caller. [Contract](../../../doc/CHANNEL_ADAPTATION.ru.md). |
| [`fvm_sadapt_commit`](../../../include/flashvm/sadapt.h#L125) | Подтверждает точный согласованный профиль после ACK; несовпадение прерывает пример. [Contract](../../../doc/CHANNEL_ADAPTATION.ru.md). |

```sh
# Controlled demonstration input; replace with real trusted host measurements.
python3 flashVM/examples/player/frame_parts/make_feedback.py reports.srecords
./build-frame-parts/frontend/flashvm_example_frame_parts --feedback reports.srecords frames.sparts
./build-frame-parts/frontend/flashvm_example_frame_parts --live --feedback reports.srecords live.sparts
```

[make_feedback.py](make_feedback.py) создаёт заданную трассу из трёх окон без импорта тестов или загрузки SDK. Ожидаемый stdout адаптивного запуска: `exported_frames=4`.
The independent test also derives native reports from actual UDP arrivals and monotonic host time, measures TCP echo RTT, then drives a second real player export. / Независимый тест также строит native отчёты по настоящим UDP поступлениям и монотонному времени, измеряет TCP echo RTT и управляет повторным экспортом настоящего плеера. [Contract](../../../doc/RECEIVE_WINDOWS.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_rdispatch_cancel` | function | [include/flashvm/rdispatch.h:149](../../../include/flashvm/rdispatch.h#L149) |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:109](../../../include/flashvm/rdispatch.h#L109) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:163](../../../include/flashvm/rdispatch.h#L163) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:128](../../../include/flashvm/rdispatch.h#L128) |
| `fvm_rdispatch_post` | function | [include/flashvm/rdispatch.h:139](../../../include/flashvm/rdispatch.h#L139) |
| `fvm_rdispatch_pump` | function | [include/flashvm/rdispatch.h:145](../../../include/flashvm/rdispatch.h#L145) |
| `fvm_rdispatch_take` | function | [include/flashvm/rdispatch.h:156](../../../include/flashvm/rdispatch.h#L156) |
| `fvm_sadapt_commit` | function | [include/flashvm/sadapt.h:125](../../../include/flashvm/sadapt.h#L125) |
| `fvm_sadapt_config_init` | function | [include/flashvm/sadapt.h:104](../../../include/flashvm/sadapt.h#L104) |
| `fvm_sadapt_feedback_read` | function | [include/flashvm/sadapt.h:139](../../../include/flashvm/sadapt.h#L139) |
| `fvm_sadapt_feedback_write` | function | [include/flashvm/sadapt.h:135](../../../include/flashvm/sadapt.h#L135) |
| `fvm_sadapt_init` | function | [include/flashvm/sadapt.h:110](../../../include/flashvm/sadapt.h#L110) |
| `fvm_sadapt_observe` | function | [include/flashvm/sadapt.h:118](../../../include/flashvm/sadapt.h#L118) |
| `fvm_sbindings_payload_write` | function | [include/flashvm/sbindings.h:51](../../../include/flashvm/sbindings.h#L51) |
| `fvm_scontrol_active` | function | [include/flashvm/scontrol.h:132](../../../include/flashvm/scontrol.h#L132) |
| `fvm_scontrol_commit` | function | [include/flashvm/scontrol.h:108](../../../include/flashvm/scontrol.h#L108) |
| `fvm_scontrol_init` | function | [include/flashvm/scontrol.h:86](../../../include/flashvm/scontrol.h#L86) |
| `fvm_scontrol_propose_at` | function | [include/flashvm/scontrol.h:123](../../../include/flashvm/scontrol.h#L123) |
| `fvm_scontrol_read` | function | [include/flashvm/scontrol.h:79](../../../include/flashvm/scontrol.h#L79) |
| `fvm_scontrol_respond` | function | [include/flashvm/scontrol.h:99](../../../include/flashvm/scontrol.h#L99) |
| `fvm_scontrol_write` | function | [include/flashvm/scontrol.h:75](../../../include/flashvm/scontrol.h#L75) |
| `fvm_sinteger_write` | function | [include/flashvm/srecord.h:62](../../../include/flashvm/srecord.h#L62) |
| `fvm_srecord_write` | function | [include/flashvm/srecord.h:76](../../../include/flashvm/srecord.h#L76) |
<!-- source-links:end -->
