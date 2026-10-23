# Раздельный dequeue-учёт BASE/TAIL

[`fvm_swf_sound_mix_queue_storage_required_for_modes`](../include/flashvm/swf_sound_mix_queue.h#L60) рассчитывает native owner/PCM и опциональные ID/BASE-TAIL bitmap. Оба флага Boolean; неверные флаги/геометрия дают INVALID_ARGUMENT с сохранением out, переполнение размера — OUT_OF_MEMORY. Аллокаций/callback/VM/GC нет. Bitmap добавляет 32768 байт для FIFO до 262144 кадров; mixer/voice/backend/allocator/VM исключены.

v0.1038: Добавлен точный расчёт памяти BASE/TAIL origin-учёта без изменения смысла прежних predictors.


v0.1039: Добавлены фиксированный origin bitmap, проверенный dequeue и точные BASE/TAIL счётчики; legacy callbacks сохранены.


v0.1040: Проверены TAIL-классификация retry, stop/failed clear, неверный рост FIFO, рекурсия и исключения callbacks.


v0.1041: Проверены wraparound, ограничение до render и последние точные кадры эпохи на явно заданных нативных метаданных.


v0.1042: Проверены BASE/TAIL парного конвертированного stream; часы используют фактический допуск BASE без ошибочного callback после write.


v0.1043: Проверены rollback выделения bitmap и BASE/TAIL retry, наблюдения и seek reset без последующих аллокаций.


v0.1044: Проверены 256 эпох с точным PCM, held TAIL, converted stream EOF и согласованными раздельными dequeue-суммами.


v0.1045: Origin-учёт включён в root composition; общий ID/bitmap бюджет проверяется до открытия SDL endpoint.


v0.1046: Добавлены сериализованные origin diagnostics и точные BASE/TAIL счётчики вместе с FWS/CWS CLI PCM в обоих VM-режимах.


v0.1047: Проверены host pause, root Stop TAIL dequeue, source phase, BASE resume, GC/native JIT и подтверждённая отмена origin-эпохи.


v0.1048: Десятый C89-режим показывает bounded origin tracking, точный gated PCM, retry и раздельные accepted/drained категории.


## Режим и владение эпохой

[`fvm_swf_sound_mix_queue_set_origin_tracking`](../include/flashvm/swf_sound_mix_queue.h#L84) выделяет фиксированный bitmap при
активных нулевых общих часах без pending retry. Будущие голоса уже могут существовать,
принятой истории быть не должно. Отключение — после проверенного FIFO drain без
retry. Повторный выбор режима не выделяет память. Enable сравнивает budget с
owner/PCM/текущей ID-table/bitmap; OUT_OF_MEMORY сохраняет режим. ID и origins
независимы; перед clock begin/end оба освобождаются в своих безопасных точках.
Подтверждённый adapted.clear сбрасывает category counters/ring/pending, сохраняя
выделенные режимы. Failed clear сохраняет владение. При ID-tracking действует
прежняя политика отмены voice/ID после подтверждённого clear.

Null/busy/неверный Boolean/state дают INVALID_ARGUMENT, ошибки backend передаются,
исключения callbacks дают ERROR. Успешный опрос может обновить played даже при
последующей ошибке budget/write/control. Аллокация только в явной настройке режима.
Все queue/mixer/backend живут до завершения вызова и сериализуются извне; VM/GC
указателей нет.

## Принятые и извлечённые категории

| Origin | Какие кадры считаются | Смысл played/dequeued |
| --- | --- | --- |
| BASE | Все принятые adapted.write кадры, включая нулевую base и примешанные события | Эти BASE-позиции покинули software FIFO |
| TAIL | Все принятые explicit event-only кадры, включая выбранный padding/release endpoint | Эти TAIL-позиции покинули software FIFO |

[`fvm_swf_sound_mix_queue_origin_info`](../include/flashvm/swf_sound_mix_queue.h#L91) требует точный sizeof(out) и включённый режим.
Один проверенный FIFO-опрос копирует BASE/TAIL submitted/played/queued. Выключенный
режим — UNSUPPORTED, неверный/reentry — INVALID_ARGUMENT, backend/clock ошибки
передаются, исключения — ERROR. При ошибке out сохраняется. Аллокаций, render и
source advancement нет. Суммы категорий равны общим accepted/dequeued/backlog
с точностью integer frames до [FVM_SWF_SOUND_OUTPUT_MAX_CLOCK](../include/flashvm/swf_sound_output.h#L14). Отклонённый render не считается до OK;
его BASE/TAIL сохраняется при gate/stop/retry через совместимый tail API. Категория
определяется операцией подачи, не значениями PCM.

Кольцо хранит до [FVM_SWF_SOUND_MIX_QUEUE_ORIGIN_FRAMES](../include/flashvm/swf_sound_mix_queue.h#L54) принятых FIFO-позиций. Tracked write наблюдает
dequeue до render; недостаток места даёт NOT_FOUND без продвижения голосов и
изменения pending. После OK запись bits/counters не вызывает ошибочный callback.
Неверный рост/регрессия отклоняются до retirement журнала. Парный stream-снимок
получает фактическое последнее наблюдение допуска, даже если backend потребляет
между опросами. Последующее потребление видно при следующем наблюдении.

## Desktop и C89

Desktop root composition рассчитывает и включает ID/origins до запуска worker.
Bitmap 32 KiB входит в FVM_DESKTOP_AUDIO_MIX_BYTES; source/voice/device/VM/allocator учитываются
отдельно. Недостаточный общий бюджет отклоняется до SDL, включая случай, когда
старый ID-only бюджет помещался. `fvm_desktop_audio_origin_info` копирует свежий
снимок под worker mutex, требует initialized composition и сохраняет out при
ошибках; главный поток не производит PCM.

`flashvm_player --stats movie.swf` печатает audio_base_submitted/dequeued и
audio_tail_submitted/dequeued для composition. --finite-audio-tails и прежний
--finite-mixed-tails сохраняют конечную политику; в default принятый final padding
остаётся TAIL. Root Stop/gates позволяют dequeue TAIL при удерживаемых converter
phase и BASE. Play продолжает сохранённую сетку. Подтверждённый StopSounds/seek
clear начинает новую category epoch по прежней cancellation-политике. Host pause
независим. C89 swf_stream_mix --origins экспортирует 17 точных кадров с BASE9/TAIL8
до/после явного file-consumer drain; десятый режим проверяет рациональный PCM.

Счётчики классифицируют операции вывода. Восстановление SWF sample positions,
decoder delay, исходных media timestamps, физических DAC/A-V/drift, автоматический
seek, дочерние словари/таймлайны, полная codec/reference latency и другие backend/
платформы/устройства остаются открытыми, как все девять стадий. White-box тесты
задают граничные метаданные, не моделируют многолетний/огромный PCM device run.
Общий mutable owner по-прежнему требует внешней сериализации.


v0.1049: Уточнены origin budget, accepted/pending ownership, точные dequeue категории, desktop diagnostics и десять PCM-режимов.


[`FVM_SWFSoundMixQueueOriginInfo`](../include/flashvm/swf_sound_mix_queue.h#L64) — копируемый снимок: submitted/played хранят точные integer-valued doubles, queued — полные native output кадры. Результат принадлежит вызывающему, заимствованного указателя нет.


## v0.1050 verification / Проверка

Origin-часы BASE/TAIL v0.1038–v0.1050: опциональный фиксированный bitmap 32 KiB учитывает принятые origin-позиции до проверенного dequeue. Adapted.write — BASE, включая нулевой PCM и примешанные события; explicit event-only pump — TAIL, включая padding/release. Pending retry не считается до OK и сохраняет origin при gate/stop/смене tail API. Новые storage/mode/observation API сохраняют смысл старых predictors и untracked callbacks. Место проверяется до render; после OK нет аллокаций или ошибочного callback. Парный stream получает фактическое последнее наблюдение допуска BASE. Desktop mixed-acquisition проверяет и включает журнал в прежнем adapter budget; --stats сообщает раздельные accepted/dequeued категории. Прошли 211 native regression/tutorial/CLI проверок, 65 выбранных ASan/UBSan плюс тринадцать --no-throw запусков и восемь SDK-free core/tutorial проверок. Дополнительные mixed compatibility и options/help проверки проходят. Исключения/ошибки выделений проверены обычной сборкой; LeakSanitizer отключён. Есть seeded wrap/capacity/2^53 проверки без огромного PCM/длительного device run, allocation rollback, отсутствие аллокаций, точный converted PCM и 256 эпох на четырёх независимых OS-потоках. Настоящий SDL dummy worker проверяет host pause, root Stop TAIL drain, сохранённую source grid, BASE resume, GC/native JIT и подтверждённую отмену эпохи. CLI FWS/CWS проверяет PCM и origin-суммы в обоих VM-режимах. Десятый C89 --origins проверяет 17 кадров, BASE9/TAIL8 и пять retry независимым рациональным оракулом. Отделённый переименованный компонент конфигурируется с явным SDK; скопированный десятимодовый checker проходит на native-бинарнике, отдельной сборки не было. Origin-учёт не восстанавливает SWF/DAC timestamps и не выполняет автоматическую A/V/drift/seek коррекцию. Дочерние словари/таймлайны, полная codec/reference latency, другие backend/платформы/устройства и все девять стадий остаются открытыми.


v0.1050: Зафиксированы bounded origin ownership, согласованные парные часы, desktop stats и PCM-проверки с открытыми A/V-границами.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_SWF_SOUND_MIX_QUEUE_ORIGIN_FRAMES` | macro / define | [include/flashvm/swf_sound_mix_queue.h:54](../include/flashvm/swf_sound_mix_queue.h#L54) |
| `fvm_swf_sound_mix_queue_origin_info` | function | [include/flashvm/swf_sound_mix_queue.h:91](../include/flashvm/swf_sound_mix_queue.h#L91) |
| `fvm_swf_sound_mix_queue_set_origin_tracking` | function | [include/flashvm/swf_sound_mix_queue.h:84](../include/flashvm/swf_sound_mix_queue.h#L84) |
| `fvm_swf_sound_mix_queue_storage_required_for_modes` | function | [include/flashvm/swf_sound_mix_queue.h:60](../include/flashvm/swf_sound_mix_queue.h#L60) |
| `FVM_SWF_SOUND_OUTPUT_MAX_CLOCK` | macro / define | [include/flashvm/swf_sound_output.h:14](../include/flashvm/swf_sound_output.h#L14) |
| `FVM_SWFSoundMixQueueOriginInfo` | struct | [include/flashvm/swf_sound_mix_queue.h:64](../include/flashvm/swf_sound_mix_queue.h#L64) |
<!-- source-links:end -->
