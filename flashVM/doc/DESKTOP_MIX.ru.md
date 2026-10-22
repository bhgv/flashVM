# Desktop-композиция корневого stream и событий

Текущий объём: desktop-владелец принимает поддержанные корневые stream-сегменты
вместе с root StartSound. Base PCM поступает через tracked native-адаптер в прежнюю
SDL-очередь. Микшер складывает все событийные голоса перед единственным насыщением
S16. Event-only и stream-only пути сохраняются. Это ограниченная root-интеграция,
а не завершение мультимедиа SWF.

## Владение и публикация кадра

`fvm_desktop_audio_open_paused` готовит bounded root-snapshots на VM-потоке,
создаёт микшер и optional [`FVM_SWFSoundMixQueue`](../src/swf/sound_mix_queue.cpp#L13), начинает пустую общую эпоху,
включает ID и подключает stream-producer к адаптеру. Worker читает только native
копии и не хранит VM/GC-указателей. Операции микшера, producer и controls используют
общий SDL mutex.

`fvm_desktop_audio_validate` и open используют один predictor отдельного потолка
`FVM_DESKTOP_AUDIO_MIX_BYTES` (по умолчанию 1 MiB). Включены owner адаптера, оба
PCM-буфера и ID-таблица; allocator, mixer/voices, snapshots, trace и device считаются
отдельно. Прежние event/source-бюджеты независимы. Недостаточный composition-бюджет
отвергается preflight до окна/устройства. Partial-владение освобождает обычный cleanup.

Root-callback готовит временный голос вне mutex, затем принимает под mutex.
NoMultiple и stop по ID учитывают pending/FIFO даже после удаления голоса из микшера.
Навигация сбрасывается до принятия событий нового кадра. Пока callbacks готовят
голоса, разрешённый кадр равен нулю: worker не подаёт ни base, ни tail. После полного
кадра владелец вызывает `fvm_desktop_audio_timeline` и публикует реальное разрешение.
Быстрый worker не может записать новый base раньше всего пакета событий кадра.
Нельзя заменять разрешение tick-счётчиком или публиковать его посреди delivery.

## EOF-хвосты и управление

Worker переходит к event-only хвостам только после финального EOF всей очереди
источников. Между источниками сохраняются порядок, frame gate и сброс фазы.
При ожидании будущего stream-кадра событийная конверсия ждёт вместе с base;
независимое продвижение событий по wall-clock в stream gaps остаётся открытым.
Авторский stop корня и host suspension удерживают композицию согласно текущей
stream pause-policy. Эталонная Flash pause/seek-семантика и точность момента старта
не заявлены: новый голос начинает после уже принятого/pending PCM.

[`fvm_swf_stream_output_pump_mix_tail`](../include/flashvm/swf_stream_output.h#L85) согласует финальный EOF-tail с обеими эпохами,
не добавляя полей в публичные структуры. Нужны наблюдённый EOF, отсутствие base retry,
точная adapted-таблица (callbacks/userdata/геометрия) данного mix queue и равные
accepted-счётчики активной общей эпохи. Используются block/target producer. Успех
учитывает хвост в обоих счётчиках; [`FVM_SWFStreamOutputInfo`](../include/flashvm/swf_stream_output.h#L12) остаётся допустимым.
EOF источника, converter и последний source chunk не меняются. Padding последнего
блока входит в submitted PCM, но не выдаётся за source samples или DAC-время.

| API | Статусы, владение и исполнение |
| --- | --- |
| [`fvm_swf_stream_output_pump_mix_tail`](../include/flashvm/swf_stream_output.h#L85) | Живые stream/mix обязательны; optional accepted обнуляется сразу. Null/reentry, ненаблюдённый EOF, base retry, иной backend или разные эпохи: INVALID_ARGUMENT. Общий clock выключен: NOT_FOUND. Нет голосов/headroom: NOT_FOUND. Прочие backend/mixer статусы передаются; исключения callbacks: ERROR. Без выделений/VM/GC, передачи владения и неявных pause/clear. Нужна внешняя сериализация всех зависимостей. |
| `fvm_desktop_audio_stream_pending` | Учитывает разрешённый source, FIFO, mixed voices и сохранённый mix retry. Host suspension и авторский stop возвращают ноль; после StopSounds новые события могут продлить completion, не включая отменённый base. Без pump/decode/VM; mutex, выход сохраняется при ошибке. |

При failed tail write микшер продвигается один раз, точный TAIL retry хранит mix queue.
Base-буфер producer не используется под хвост; `FVM_SWFStreamOutputInfo.pending_frames`
по-прежнему описывает только base retry. Для хвоста нужен
[`fvm_swf_sound_mix_queue_pending`](../include/flashvm/swf_sound_mix_queue.h#L31). Допустимый FIFO poll может обновить played при
ошибке операции; failed write не увеличивает accepted. Нельзя чередовать paired
хвосты с прямым mix-tail или посторонним producer. Перед
[`fvm_swf_stream_output_continue`](../include/flashvm/swf_stream_output.h#L125) нужно завершить tail retry; подтверждённый seek/clear
его отменяет. Разные эпохи отвергаются до следующего смешивания.

Context StopSounds выполняет staged stream seek/clear через адаптер. Успех отменяет
base/mix retry, голоса, ID и оба счётчика, удерживая base до навигации. Ошибка clear
сохраняет это состояние; успешная pause остаётся видимой. Новое root-событие может
играть после отмены без включения старого base и последующих headers. Навигация
начинает новую эпоху; замена корня отсоединяет callbacks и join worker до освобождения.

## Проверки и оставшаяся работа

Исполняемые C89/C++98 fixtures проверяют точный PCM, failed base/tail write,
согласованные часы, сложение противоположных голосов до насыщения, переходы и reset
фазы, future gates, queued NoMultiple, failed clear/retry, pause, rewind, настоящий
root StopSounds, GC и interpreter/native-JIT entries. Настоящий SDL worker проверяет
публикацию кадра до и после EOF на явно выбранном dummy SDK. Детерминированные FIFO
тесты компилируют production C89 worker; время физического устройства не заявлено.
VM уничтожается до завершения одной из native-конверсий.

Дочерние словари/таймлайны, независимые события в stream gaps, полная codec/reference
latency/seek, автоматический A/V drift, все JIT backends, приёмка concurrency/GC/debugger,
физические устройства/платформы и все девять стадий остаются открытыми.
Исторические записи ниже не расширяют этот объём.


v0.975: Добавлен [fvm_swf_stream_output_pump_mix_tail](../include/flashvm/swf_stream_output.h#L85) после EOF: точный retry и согласованные счётчики без изменения публичных структур.


v0.976: Проверены pairing backend, failed/throwing retry, stream-reentry и запрет рассогласованных эпох.


v0.977: Добавлены bounded-владение composition и маршрутизация ID; mixed preflight пока закрыт до интеграционных проверок.


v0.978: Разрешена bounded root-композиция stream/StartSound; навигация согласуется до принятия событий кадра, прежние пути сохранены.


v0.979: Worker подаёт согласованные хвосты после финального EOF, учитывает active/pending события и сохраняет StopSounds-остановку stream.


v0.980: Production C89 refill проверен на явном FIFO: base retry, 5 кадров base+event и 4 кадра хвоста с точными PCM.


v0.981: Root-действия проверены interpreter/native JIT: queued NoMultiple, failed clear, GC и новые события после StopSounds.


v0.982: Проверены pending EOF-tail, пауза без rewind, failed navigation clear и свежие события в новой эпохе.


v0.983: Base и EOF-tail удерживаются до конца frame callbacks; проверен настоящий SDL worker до и после stream EOF.


v0.984: Один predictor используется в preflight/open: отдельный composition-бюджет отвергается до устройства, partial-владение очищается.


v0.985: Проверены переходы источников, future gates, rewind-эпохи и точный PCM противоположных голосов в C89 worker после VM teardown.


Непрерывная stream-интерполяция может читать следующий immutable sample того же подготовленного header, даже когда запись его блока ещё не разрешена. Frame gate ограничивает владение/запись блоков, а не такой lookahead. Исправленный ADPCM future-gate oracle считает полную рациональную сетку header и сверяет разрешённый префикс; ранний host stop не превращает источник в EOF.


## v0.986 verification

Desktop root-композиция v0.975–v0.986: root stream и StartSound используют bounded tracked-очередь, одно насыщение, NoMultiple/stop, staged StopSounds/seek и финальные EOF-хвосты с согласованными счётчиками. Worker получает кадр после всех owner callbacks, в том числе после EOF. Всего прошли 163 разные native проверки с примерами/CLI (старый ADPCM early-stop oracle исправлен на непрерывный same-header префикс и повторно проверен); 31 выбранный ASan/UBSan и восемь --no-throw запусков. Один sanitizer-бинарник сначала не имел execute permission и прошёл после исправления прав. Дополнительно прошли три SDK-free проверки ядра/примера. FWS/CWS mixed CLI в обоих VM-режимах совпал с 1033 точными stereo-кадрами с padding; root-action тесты проверяют native JIT entries и GC safe points. SDL SDK статический dummy-only, приёмка физического окна/устройства не заявлена. LeakSanitizer отключён; исключения/allocation-faults — обычная сборка. Переименованный отделённый компонент сконфигурирован с явным SDL SDK, скопированный six-mode PCM-checker прошёл на native-бинарнике; отдельной сборки не было. Независимые события в stream gaps/авторской pause, дочерние словари/таймлайны, reference codec/seek/A-V и все девять стадий остаются открытыми. [Контракт](DESKTOP_MIX.ru.md).


v0.986: Завершены контракты root-композиции, mixed CLI oracle и continuous ADPCM prefix; зафиксированы проверки и оставшаяся работа.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_pending` | function | [include/flashvm/swf_sound_mix_queue.h:31](../include/flashvm/swf_sound_mix_queue.h#L31) |
| `fvm_swf_stream_output_continue` | function | [include/flashvm/swf_stream_output.h:125](../include/flashvm/swf_stream_output.h#L125) |
| `fvm_swf_stream_output_pump_mix_tail` | function | [include/flashvm/swf_stream_output.h:85](../include/flashvm/swf_stream_output.h#L85) |
| `FVM_SWFSoundMixQueue` | struct | [src/swf/sound_mix_queue.cpp:13](../src/swf/sound_mix_queue.cpp#L13) |
| `FVM_SWFStreamOutputInfo` | struct | [include/flashvm/swf_stream_output.h:12](../include/flashvm/swf_stream_output.h#L12) |
<!-- source-links:end -->
