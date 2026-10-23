# Событийный звук при удержании stream-кадров

Native и desktop mixed-пути теперь продолжают события, пока base stream ждёт
будущий movie-кадр или корневой таймлайн авторски остановлен. Это расширяет
v0.986, где event-only работа разрешалась лишь после финального EOF. Дочерние
потоки и эталонная Flash A/V/seek-семантика сюда не входят.

## Native scheduler

[`fvm_swf_stream_output_pump_mix_until`](../include/flashvm/swf_stream_output.h#L111) согласует один шаг stream и его точного
[`FVM_SWFSoundMixQueue`](../src/swf/sound_mix_queue.cpp#L13). Нужны живые владельцы с общей adapted-таблицей, геометрией,
активным clock и равным accepted-счётчиком. Unsigned movie frame разрешает base;
ноль удерживает все base-кадры, но допускает события. Tick не превращается в
movie/device-время автоматически. Вызывающий сериализует stream, converter,
snapshot, mixer, mix queue и exclusive backend. Передачи владения, VM и GC нет.

Сначала принимается сохранённый TAIL retry с прежними байтами, даже если gate уже
разрешает base. Затем приоритетен разрешённый base. Если следующий source-кадр
удержан или достигнут EOF, голоса складываются с нулевой базой. Backpressure
разрешённого base не считается gate и не разрешает подмену. Сохранённый BASE
удерживается при запрете его исходного кадра: нельзя молча отменить продвижение
голосов или пересмешать его байты.

| API | Результаты и выходные данные |
| --- | --- |
| [`fvm_swf_stream_output_pump_mix_until`](../include/flashvm/swf_stream_output.h#L111) | Optional accepted обнуляется сразу. OK принимает один блок либо нулевой marker источника. Неверный stream/reentry/backend/эпоха: INVALID_ARGUMENT. Нет общего clock/headroom/голосов или удержан BASE: NOT_FOUND; для EOF нужны producer/mix pending, одного статуса недостаточно. Прочие native/backend статусы передаются; исключения callbacks: ERROR. Без выделений, VM, GC, неявных pause/clear и автоматического A/V retiming. |
| [`fvm_swf_sound_mix_queue_pending`](../include/flashvm/swf_sound_mix_queue.h#L31) | Без callbacks показывает владение BASE/TAIL. Failed event-only блок хранится здесь отдельно от base PCM producer. |
| [`fvm_swf_stream_output_info`](../include/flashvm/swf_stream_output.h#L126) | Свежие accepted/dequeued включают успешные base и event-only блоки. Source chunk/EOF и base pending сохраняют прежний смысл. Сериализация со всеми scheduling/controls; ошибка сохраняет выход. |

Нулевая база использует прежние block/target и оба точных счётчика, не меняя
позицию, фазу, chunk и EOF источника. Финальный padding — настоящий принятый output,
а не дополнительные source samples. Без голосов и mix retry новые блоки не
создаются: удержание stream не порождает бесконечную тишину. Failed write сохраняет
точный микс и не увеличивает accepted; допустимый poll может обновить played.
После отказа нужно продолжать этим scheduler либо отменять подтверждённым seek/clear.
Посторонние прямые writes/mix-tail нарушают exclusive accounting.
Прежний [`fvm_swf_stream_output_pump_mix_tail`](../include/flashvm/swf_stream_output.h#L92) всё ещё требует наблюдённый EOF.

## Desktop frame/pause policy

Worker переключает исчерпанные сегменты до event-only работы на gate следующего
источника и не переключается при сохранённом TAIL retry. Признак завершённого
кадра отделён от разрешения base: пока VM callbacks готовят все события кадра,
оба producer удерживаются. После кадра `fvm_desktop_audio_timeline` публикует
готовность. Авторский root Stop задаёт только base gate=0: события могут prime,
resume и drain настоящую очередь. Host/session pause по-прежнему останавливает
весь output. NoMultiple включает accepted/pending ID до dequeue.

`fvm_desktop_audio_stream_pending` учитывает source, FIFO, события и processable
mix retry также при authored Stop. BASE с запрещённым исходным кадром не может
продвинуться; после drain принятого lead он не заставляет finite completion ждать
бесконечно. Для разрешения нужны resume или подтверждённый seek/clear. Уже принятый
mixed lead может доиграть при root Stop: выборочно размешать его нельзя. Обычный
stream-only pause-путь сохраняется.

Context StopSounds сохраняет staged full cancellation: failed clear удерживает
оба retry и ID; успешный сбрасывает обе эпохи и голоса. Новые события могут играть,
не включая отменённый base. Seek/замена корня сохраняют прежнюю cancellation-policy,
не эталонное сохранение событий. Event-only время не продвигает source samples:
stream продолжит прежнюю сетку после уже queued событий. Drift correction,
выравнивание timestamps и sample-accurate seek из этих счётчиков не следуют.

## Исполняемые проверки и ограничения

C89 `swf_stream_mix --gated` экспортирует после VM teardown 17 точных stereo-кадров:
восемь event-only, затем девять base. Каждый write отклоняется; retry сразу открывает
gate и проверяет TAIL priority. Seven-mode checker использует независимый рациональный
PCM oracle. Native-тесты проверяют оба направления gate, NOT_FOUND write failures,
throwing callbacks, reentry, отсутствие выделений, seeded границы 2^53 и 256 циклов
на четырёх независимых OS-потоках. White-box предел не является длительным device run.

Desktop-тесты проверяют удержанный BASE/принятый lead, failed StopSounds clear, GC,
остановленный root и interpreter/native JIT entries. Настоящий SDL producer проверяет
host pause, drain остановленного root и последующий Play без изменения source phase.
Независимый CLI WAV-oracle покрывает FWS/CWS, оба VM-режима, authored Stop/future gates
и mute без аудиоустройства. SDK содержит только dummy audio/video; физические
устройства/окна, платформы и временная точность не считаются проверенными.

Дочерние словари/таймлайны, полная codec/reference latency, sample-clock A/V,
drift/seek, все JIT backends, широкая приёмка concurrency/GC/debugger и все девять
стадий остаются открытыми. Ниже сохранены исторические checkpoints.


v0.987: Native stream work вынесен под единый callback barrier; base retry, frame gates и EOF-tail сохранены.

v0.988: Добавлен paired pump_mix_until: разрешённый base или event-only работа при сохранении source position и согласованных часов.

v0.989: Проверены TAIL retry перед новым base, удержание BASE, callback-reentry и исключения backend.

v0.990: Mixed worker использует paired gate scheduling: события идут через future gates, retry приоритетен, источники переключаются до event-only подачи.

v0.991: Публикация завершённого кадра отделена от base-разрешения: root Stop держит stream phase, события продолжаются, host pause останавливает очередь.

v0.992: Проверены seeded границы 2^53, backend/clock rejection и отсутствие выделений в gate/retry/clear после подготовки.

v0.993: Выполнены 256 native-эпох на четырёх OS-потоках: VM teardown, failed tail retry, смена gates, точный PCM и dequeue/ID.

v0.994: Удержанный BASE сохранён; проверены drain принятого lead, failed clear и новые события остановленного корня после staged cancellation.

v0.995: Добавлен седьмой gated-режим C89: 17 точных кадров, неизменный converter при удержании и failed TAIL-to-BASE переходы.

v0.996: Настоящий SDL worker проверен: host pause, authored Stop, GC/native JIT, event-only drain и последующий base resume без rewind.

v0.997: Проверен точный event-only WAV FWS/CWS в обоих VM-режимах при future gate и root Stop, а также mute без аудиоустройства.


## v0.998 verification

Mixed event gates v0.987–v0.998: paired pump_mix_until продвигает события при future stream gates и authored root Stop, сохраняя source phase, точный порядок BASE/TAIL retry и оба счётчика. Публикация завершённого кадра удерживает producer во время callbacks; host pause держит всю очередь. Удержанный BASE не подменяется: нужны resume или подтверждённая отмена; принятый mixed lead не удаляется выборочно. Прошли 172 native проверки с примерами/CLI, 37 выбранных ASan/UBSan и девять --no-throw запусков, восемь SDK-free проверок ядра/примера. Allocation/exception paths — обычная сборка; LeakSanitizer отключён. Native-тесты включают seeded предел 2^53, отсутствие выделений и 256 циклов на четырёх независимых OS-потоках; shared-owner без синхронизации и длительный device run не заявлены. Прошли root actions/native JIT/GC, настоящий SDL worker остановленного корня, точный FWS/CWS CLI WAV/mute и seven-mode C89 PCM oracle. Отделённый переименованный компонент сконфигурирован с явным SDK, его скопированный checker прошёл на native-бинарнике; отдельной сборки не было. SDL — dummy-only. Дочерние stream/словари, полная codec/reference latency, sample-clock A/V/drift/seek, все backend/platform/device acceptance и девять стадий остаются открытыми. [Контракт](MIXED_GATES.ru.md).


v0.998: Завершены C89/API и desktop-контракты authored Stop, ordered retry, gated PCM и оставшихся reference/device ограничений.


## Неполный остаток FIFO с v0.1000

Парные gate/EOF операции используют [`fvm_swf_sound_mix_queue_pump_tail_up_to`](../include/flashvm/swf_sound_mix_queue.h#L262): новые
event-only блоки заполняют свободный остаток до настроенного block. Короткий TAIL
неделим и остаётся раньше вновь разрешённого BASE. Обе копии часов используют
наблюдение, разрешившее успешный write, без ошибочного опроса после принятия.
Старый API точного размера не изменён. [Полный контракт](ADAPTIVE_TAILS.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_pending` | function | [include/flashvm/swf_sound_mix_queue.h:31](../include/flashvm/swf_sound_mix_queue.h#L31) |
| `fvm_swf_sound_mix_queue_pump_tail_up_to` | function | [include/flashvm/swf_sound_mix_queue.h:262](../include/flashvm/swf_sound_mix_queue.h#L262) |
| `fvm_swf_stream_output_info` | function | [include/flashvm/swf_stream_output.h:126](../include/flashvm/swf_stream_output.h#L126) |
| `fvm_swf_stream_output_pump_mix_tail` | function | [include/flashvm/swf_stream_output.h:92](../include/flashvm/swf_stream_output.h#L92) |
| `fvm_swf_stream_output_pump_mix_until` | function | [include/flashvm/swf_stream_output.h:111](../include/flashvm/swf_stream_output.h#L111) |
| `FVM_SWFSoundMixQueue` | struct | [src/swf/sound_mix_queue.cpp:13](../src/swf/sound_mix_queue.cpp#L13) |
<!-- source-links:end -->
