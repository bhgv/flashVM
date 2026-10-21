# Адаптация bulk-профиля по обратной связи

[English](CHANNEL_ADAPTATION.md). `sadapt.h` — C89 policy без выделений между
измерениями хоста и [tag-zero согласованием](MEDIA_SLOTS.ru.md). Модуль выбирает
поддерживаемый профиль, но сам не измеряет сеть и не меняет live framing.
Мелкие управляющие записи остаются компактными.

## Измерения и профили

Хост задаёт лестницу из 1..8 bulk-профилей с одинаковым привязанным tag и строго
возрастающими slot_bytes, part_bytes, record_limit. Каждый проходит правила
scontrol для ёмкости/запасов. Лестница должна подходить обоим peers, буферам и
path policy. Init копирует её; input table после этого не заимствуется.
Начальный индекс — реально committed профиль. Направления/epoch имеют разные policy.

Report содержит stream/epoch и возрастающий ненулевой Uint32 window. Expected —
число ожидаемых частей до deadline хоста; on_time — уникальные пригодные вовремя,
late — отдельные опоздавшие, damaged — отдельные повреждённые. Категории не
перекрываются и суммарно не больше expected. Missing вычисляется; дубликаты не
samples. Всё непригодное к deadline (late/damaged/missing) считается потерей для
выбора профиля. Expected известно из доверенных object metadata/deadlines;
пропуск ID обратной связи сам по себе не означает потерю пакетов. Если IP/UDP
отбросил повреждённую датаграмму до приложения, она попадёт в missing.

RTT явно измеряется в миллисекундах. При unknown flag значение обязано быть 0;
неизвестный RTT запрещает рост, но не уменьшение по потерям. Хост аутентифицирует
feedback: CRC этого не делает. Чужая epoch, старый/повторный window и report при
pending negotiation дают CONFLICT без изменений. Неверные counts дают INVALID.

## Решения, задержка переключений и ACK

Плохое окно достигает порога потерь либо высокого известного RTT. Последовательные
плохие окна предлагают соседний меньший профиль. Хорошее окно имеет низкие потери
и известный низкий RTT; более длинная устойчивая серия разрешает соседний больший.
Промежуточное/неизвестное качество сбрасывает streaks. На краю лестницы размер
не выходит за предел. Проценты сравниваются точно через деление/остаток, без
переполнения Uint32*100.

Иллюстративные defaults: bad >=10%, good <=1%, high RTT >=250 мс, low RTT <=50 мс,
два плохих/четыре хороших окна, cooldown два окна, минимум 64 expected части.
Это настраиваемый пример, не универсальная рекомендация для сети. Недостаточное
окно сбрасывает streaks и не расходует cooldown; полные окна cooldown потребляются
без изменения размера.

Предложение pending не меняет current. Хост отправляет OFFER_AT и подтверждает
коррелированный ACK на будущий объект, затем adaptation commit принимает только
точный выбранный профиль лестницы. Clamped/другая геометрия даёт CONFLICT, её
нельзя выдавать за candidate. При иных пределах нужно сверить actual peer limits
и пересобрать поддерживаемую лестницу. Reject сохраняет current, включает cooldown.
Нет feedback queue, скрытого повтора, часов и sender. Разрыв управления требует
сверки сессии до возобновления данных.

Один размер не заменяет congestion control и не гарантирует улучшения потерь или
скорости. Pacing, path MTU и congestion policy остаются отдельными. Loopback не
доказывает допустимость большого UDP в интернете. Первоисточник:
[RFC 8085](https://www.rfc-editor.org/info/rfc8085/).

## Многофункциональный tag 0

Selector 1 сохраняет привязки типов, 2 — limits. Selector 3 — feedback, command 1.
Payload — одиннадцать канонических unsigned S integers:

`3, 1, stream, epoch, window, expected, on_time, late, damaged, rtt_known, rtt_ms`

Это обычная короткая запись tag 0 переменной длины, без padding. Read потребляет
одну запись, остаток обрабатывает хост. Неизвестные службы/команды дают UNKNOWN
для явной маршрутизации. Неверный известный report — INVALID. Decode/observe
не выдаёт native права.

| API | Владение и результат |
| --- | --- |
| [`fvm_sadapt_config_init`](../include/flashvm/sadapt.h#L96) | Только defaults; хост задаёт ladder/identity/initial, без выделений/startup. |
| [`fvm_sadapt_init`](../include/flashvm/sadapt.h#L100) | Атомарно проверяет/копирует config/лестницу. State должен иметь стабильный адрес: config ссылается на его собственный массив. Нельзя memcpy/перемещать живую policy в другой адрес. |
| [`fvm_sadapt_observe`](../include/flashvm/sadapt.h#L106) | Потребляет один валидный report, копирует KEEP/INSUFFICIENT либо pending SHRINK/GROW. Ошибка сохраняет state/decision. Не вызывает I/O/negotiation/callback и не меняет framing. |
| [`fvm_sadapt_commit`](../include/flashvm/sadapt.h#L111) | После control ACK подтверждает точную pending geometry, меняет current и включает cooldown. Нет proposal/иной selected — CONFLICT. |
| [`fvm_sadapt_reject`](../include/flashvm/sadapt.h#L114) | Удаляет pending, сохраняет current, включает cooldown; peer не откатывает. |
| [`fvm_sadapt_feedback_write`](../include/flashvm/sadapt.h#L117) | Компактный tag-zero report в буфер клиента, output/used атомарны. |
| [`fvm_sadapt_feedback_read`](../include/flashvm/sadapt.h#L119) | Проверяет одну запись, копирует report только при OK. Prefix/checksum errors передаются, output/used атомарны. |

State/input/output не перекрываются, доступ последовательный. Нет VM/GC references,
скрытых потоков или изменяемого process-global состояния.

## Сборка и проверка

Модуль включён в detachable flashvm_srecord и SDK, без SDL/VM startup.
C89-contract проверяет pending/reject/cooldown, недостаточные samples, unknown RTT,
порог max Uint32 и codec. Независимый Python integer/zlib oracle строит точные
байты и моделирует 1200 окон, commit/reject и atomicity повторов. C++98 ABI сохранён.

```sh
cmake -S flashVM -B build-adapt -DFVM_BUILD_TESTS=ON
cmake --build build-adapt --target flashvm_srecord_adapt_contract flashvm_srecord_oracle
ctest --test-dir build-adapt -R flashvm_srecord_adapt --output-on-failure
```

Production listener, quality sampler, новые media codecs и автоматическая
адаптация всего плеера этим не установлены. Хост явно соединяет measurements,
control transport и применение профиля на границе кадра.

## Настоящие кадры с обратной связью (v0.746)

[Запускаемый пример frame_parts](../examples/player/frame_parts/README.ru.md)
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

Проверка v0.746: прошли 54 выбранные native-проверки и 14 проверок shared-host. Новая политика, cache/protocol и интеграция настоящих кадров interpreter/JIT прошли ASan/UBSan (проверка утечек отключена из-за ограничений хоста). Strict C89 link и C++98 consumer собираются для ARM64 Linux/musl, RISC-V64 Linux/musl и Windows x64/MinGW; чужие бинарники не запускались. Все девять стадий открыты.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_sadapt_commit` | function | [include/flashvm/sadapt.h:111](../include/flashvm/sadapt.h#L111) |
| `fvm_sadapt_config_init` | function | [include/flashvm/sadapt.h:96](../include/flashvm/sadapt.h#L96) |
| `fvm_sadapt_feedback_read` | function | [include/flashvm/sadapt.h:119](../include/flashvm/sadapt.h#L119) |
| `fvm_sadapt_feedback_write` | function | [include/flashvm/sadapt.h:117](../include/flashvm/sadapt.h#L117) |
| `fvm_sadapt_init` | function | [include/flashvm/sadapt.h:100](../include/flashvm/sadapt.h#L100) |
| `fvm_sadapt_observe` | function | [include/flashvm/sadapt.h:106](../include/flashvm/sadapt.h#L106) |
| `fvm_sadapt_reject` | function | [include/flashvm/sadapt.h:114](../include/flashvm/sadapt.h#L114) |
<!-- source-links:end -->
