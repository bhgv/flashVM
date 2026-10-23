# Конечный событийный звук без stream

[`fvm_swf_sound_output_pump_finite`](../include/flashvm/swf_sound_output.h#L133) ограничивает свежий PCM живым остатком микшера; pending retry всех режимов, включая stop_release, сохраняется целиком. Нулевые source-отсчёты и release endpoint не удаляются. accepted сначала обнуляется; отсутствие будущих голосов даёт NOT_FOUND без синтеза тишины. Владение, сериализация backend, часы и статусы/исключения совпадают с прежним pump; аллокаций, VM и GC нет. Старый pump продолжает silent priming. NOT_FOUND не означает FIFO drain или физическую слышимость.

v0.1025: Добавлен конечный event-only pump с точными последними кадрами и сохранением legacy/release/pending retry.


v0.1026: Добавлено сериализованное наблюдение voice/retry/FIFO с обновлением dequeue clock и сохранением out при ошибке.


v0.1027: Проверены failed clear/write, исключения, callback reentry и завершение anonymous release после удаления голосов.


v0.1028: Проверены NoMultiple до dequeue, последний точный кадр часов и отсутствие аллокаций при коротких retry и отмене.


v0.1029: Проверены 256 эпох с короткими retry, независимым рациональным PCM, pending и истечением queued ID.


v0.1030: Добавлены неизменный event policy, CLI finite-audio-tails и совместимые wrappers прежнего mixed-only startup.


v0.1031: Event-only вывод удерживается при подготовке и callbacks кадра, затем разрешается владельцем в безопасной точке.


v0.1032: Подключены finite event pump, resume коротких очередей, завершение после FIFO drain и исключение ожидаемой idle-паузы из underrun.


v0.1033: CLI проверяет одновременные голоса, короткие и многоблочные хвосты, FWS/CWS, native JIT, точный PCM и mute.


v0.1034: Проверены host pause, публикация кадра, root Stop, native JIT/GC, точный dequeue drain и release после StopSounds.


v0.1035: C89-пример показывает конечное исчерпание, retained legacy PCM, pending FIFO и подтверждённый software drain.


## Pending и нативное управление

[`fvm_swf_sound_output_pending`](../include/flashvm/swf_sound_output.h#L142) наблюдает будущие голоса, retained retry и свежий
принятый FIFO под одним callback-барьером output. Обновляются проверенные dequeue
часы, без render/seek/чтения source PCM. Требуются живые сериализованные output/out;
paused output тоже сообщает работу. Неверные параметры/reentry — INVALID_ARGUMENT;
ошибки backend передаются, невозможный/регрессирующий FIFO — ERROR, bad_alloc —
OUT_OF_MEMORY, прочие исключения — ERROR. Ошибки сохраняют out; успешный опрос может
обновить внутренний played. Аллокаций, VM и GC нет. Ноль — отсутствие текущей работы,
не запрет последующего старта и не DAC timestamp. NOT_FOUND от pump не означает drain.

Finite pump использует прежние буферы и бюджет. Новый блок ограничен headroom и
живым horizon; переполнение часов даёт ERROR до render. Retained PCM обоих pump
и stop_release повторяется целиком. Failed clear сохраняет его; подтверждённый stop
отменяет эпоху. NoMultiple действует до dequeue pending/accepted времени после
удаления голоса. Idle gap не создаёт искусственную тишину и не добавляется к часам.

## Конечное завершение desktop

`flashvm_player --finite-audio-tails movie.swf` включает конечные event-only и root
stream/event хвосты. Прежний --finite-mixed-tails остаётся mixed-only; defaults не
изменены. Stream-only путь сохраняет свою политику, --mute не открывает устройство.
`fvm_desktop_audio_open_policy` проверяет три Boolean-флага и записывает неизменную
политику до запуска worker. Старые acquisition-функции сохраняют поведение.

В finite event-only режиме callback владельца удерживает вывод до подготовки
источника вне mutex. `fvm_desktop_audio_timeline` публикует разрешение только после
полной доставки кадра: однокадровые события получают общую сетку. Embedding-хост
обязан публиковать кадр и при authored root Stop. Host pause удерживает весь вывод.
Ранее принятый FIFO может потребляться во время удержания новых блоков и не может
быть выборочно отозван. Worker подаёт конечные блоки, возобновляет короткий FIFO
ниже driver reserve и отличает ожидаемый конец/idle от underrun.

`fvm_desktop_audio_stream_pending` теперь также сериализованно наблюдает finite
event-only голоса/retry/FIFO, включая release без голосов. При suspended хосте
возвращается отсутствие обрабатываемой работы, не подтверждение drain или отмены.
Существующий ограниченный tail loop плеера ждёт завершения с прежними STOP/pause/
navigation/deadline правилами. Legacy event-only завершение не меняется. StopSounds
сохраняет подтверждённый clear и anonymous boundary release.

## Проверки и границы

Нативно проверены рациональный PCM, короткий retry, NoMultiple до dequeue, граничные
часы, отсутствие аллокаций и 256 эпох на четырёх независимых OS-потоках. Настоящий
SDL dummy worker проверяет публикацию кадра, host pause, root Stop, GC/native JIT,
точный drain и stop release. CLI FWS/CWS interpreter/JIT проверяет одновременные
противоположные голоса, полный короткий и многоблочный PCM длиннее queue target.
C89 swf_audio_queue --finite изолирует пустые голоса, legacy retry и software drain;
это намеренно silence protocol, не audible acceptance. Девятирежимный PCM-оракул
swf_stream_mix сохранён. Sound class API, дочерние словари/таймлайны, reference
codec latency/seek, физические DAC/A-V timestamps/drift, другие платформы/устройства
и все девять этапов остаются открытыми.


v0.1036: Уточнены pending/release, включение плеера, публикация кадра, mixed-policy совместимость и открытая приёмка.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_output_pending` | function | [include/flashvm/swf_sound_output.h:142](../include/flashvm/swf_sound_output.h#L142) |
| `fvm_swf_sound_output_pump_finite` | function | [include/flashvm/swf_sound_output.h:133](../include/flashvm/swf_sound_output.h#L133) |
<!-- source-links:end -->
