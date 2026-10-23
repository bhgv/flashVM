# Queued sound identity / Идентичность звука в очереди

Executable strict-C89 tutorial using public native APIs and an explicitly modeled
FIFO. An original embedded SWF contains seven 44.1 kHz mono PCM samples and plain/
NoMultiple starts. Prepare independent voices, destroy the VM, convert to 48 kHz
stereo, retire the software voice, and suppress a duplicate while its PCM remains
queued. Consume exactly eight voiced output frames, restart NoMultiple, reject
one backend write and retry the identical prepared bytes. Stop-by-ID releases the
registration; acknowledged whole-output clear resets the epoch.

Исполняемый пример C89 с публичными native API и явно моделируемой FIFO. Исходный
встроенный SWF содержит семь моносэмплов 44.1 kHz и plain/NoMultiple StartSound.
Голоса готовятся до уничтожения VM; далее 48 kHz stereo-выход освобождает голос
микшера, но подавляет дубликат до конца queued PCM. Ровно восемь voiced кадров
завершают ID. Повторный старт, один отклонённый write и точный retry не декодируют
и не смешивают звук заново. Stop-by-ID снимает регистрацию, общий clear — epoch.

## Build and run / Сборка и запуск

```sh
cmake -S flashVM -B build-ids \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs" \
  -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON
cmake --build build-ids --target flashvm_examples_multimedia_swf_queued_identity_main
./build-ids/flashvm_examples_multimedia_swf_queued_identity_main --help
./build-ids/flashvm_examples_multimedia_swf_queued_identity_main
./build-ids/flashvm_examples_multimedia_swf_queued_identity_main accepted.pcm
python3 flashVM/scripts/check_swf_queued_identity_example.py ./build-ids/flashvm_examples_multimedia_swf_queued_identity_main
```

C89/C++98 and bundled static core dependencies are sufficient; NO_MEDIA works.
No source file, compiler, SDL or OS device is required. No arguments executes the
finite demonstration without filesystem side effects. Optional OUTPUT.pcm is
native-endian interleaved S16, 48 kHz/stereo, exactly 32 accepted frames (two
16-frame blocks). Eight voiced frames and eight zero frames repeat identically.
Expected report contains `queued_duplicate=consumed retired=1 endpoint=8`,
`accepted_frames=32` and `after_vm_teardown=1`. identity_bytes reports actual
native ID-table storage, 512 KiB on this host.

Достаточно C89/C++98 и встроенных статических зависимостей; NO_MEDIA поддержан.
Без аргументов выполняется конечная демонстрация без файлов. Не нужны внешний SWF,
компилятор, SDL или устройство. Необязательный OUTPUT.pcm — native-endian S16,
48 kHz/stereo, ровно 32 принятых кадра: два одинаковых блока по 16, каждый из
восьми voiced и восьми нулевых кадров. Отчёт содержит указанные выше поля;
identity_bytes показывает реальный размер таблицы, 512 KiB на этом хосте.

## Ownership, limits and errors / Владение, границы и ошибки

All operations on output/mixer/backend run serially on one host thread. Output
borrows a stack-owned FIFO and live mixer; only output may render/stop it. Querying
mixer info is read-only. Backend copies complete PCM before OK, accepts nothing
on injected write failure, preserves copied evidence across clear and explicitly
reduces backlog only at the demonstrated consumption steps. There is no automatic
wall/device clock. Accepted evidence concatenates blocks, not a rendered pause/
clear timeline or audible trace. This model does not open or test a real device.

Выход/микшер/backend сериализованы одним потоком. Выход заимствует стековую FIFO
и живой микшер; чтение/остановка микшера идут только через выход. mixer_info —
неизменяющий запрос. Backend копирует весь PCM до OK, ничего не принимает при
write failure, сохраняет evidence при clear и уменьшает очередь только явно.
Автоматических часов нет: принятые блоки не являются слышимой трассой пауз/clear.
Устройство этот пример не открывает и не проверяет.

Source runtime payload is bounded to 1 MiB. Each prepared voice and the fixed/
adopted mixer charge use a separate 32 KiB ceiling; external untransferred voices
are not part of the mixer's aggregate budget. Output retry/ID storage and caller
PCM evidence are separately owned and reported. Limits exclude allocator overhead
and stdio. This is not a global process/RSS budget. All three voices survive VM
teardown. Successful adoption/suppression clears its handle; errors preserve
untransferred ownership. Close output before mixer/backend, then close remaining
voices. File open begins after every native sample/control check; late write/
flush errors may leave a partial file. Invalid CLI/native/file results return
nonzero; the one deliberate backend ERROR is checked as an expected scenario.

Runtime payload ограничен 1 MiB. Каждый подготовленный голос и память fixed/
adopted микшера отдельно ограничены 32 KiB; внешние непереданные голоса не входят
в общий бюджет микшера. Retry/ID выхода и evidence вызывающего учитываются отдельно,
без накладных расходов аллокатора/stdio. Это не общий RSS-предел. Успешная передача/
подавление обнуляет указатель; ошибка сохраняет непереданного владельца. Выход
закрывается до микшера/backend, затем оставшиеся голоса. Файл открывается после
native-проверок; поздняя ошибка может оставить частичный файл. Неожиданные ошибки
возвращают ненулевой код; единственный намеренный ERROR write проверяется явно.

## APIs in main.c / API и контракты

Detailed new contracts: [EN](../../../doc/QUEUED_SOUND_IDS.md) /
[RU](../../../doc/QUEUED_SOUND_IDS.ru.md).
[Prepared ownership](../../../doc/NATIVE_VOICES.md) and
[existing output backend](../../../doc/SWF_SOUND_OUTPUT.md) describe borrowed
lifetimes, aggregate mixer limits and atomic callback requirements.

| Function | Demonstrated behavior and lifetime |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L62) | Initialize source configuration before setting its payload ceiling |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L64) | Own source runtime through source preparation |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L68) | Own context borrowing runtime on its owner thread |
| [fvm_load_memory](../../../include/flashvm/flashvm.h#L72) | Validate/copy immutable embedded SWF; no external file dependency |
| [fvm_swf_sound_voice_open_bounded](../../../include/flashvm/swf_sound.h#L97) | Decode independent plain/NoMultiple native copies under each voice ceiling |
| [fvm_swf_sound_voice_clone](../../../include/flashvm/swf_sound.h#L111) | Deep-copy NoMultiple playback metadata/PCM without another decoder call |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L70) | Release managed source before native output is acquired |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L66) | End source runtime before native identity controls |
| [fvm_swf_sound_voice_output_frames](../../../include/flashvm/swf_sound_mixer.h#L44) | Observe exact fresh-grid duration without advancing PCM |
| [fvm_swf_sound_mixer_create_bounded](../../../include/flashvm/swf_sound_mixer.h#L65) | Check fixed mixer/slot storage before allocation |
| [fvm_swf_sound_output_create](../../../include/flashvm/swf_sound_output.h#L50) | Borrow empty exclusive FIFO and live mixer; copy callbacks and pause backend |
| [fvm_swf_sound_output_set_identity_tracking](../../../include/flashvm/swf_sound_output.h#L61) | Allocate optional fixed ID table before the first voice starts |
| [fvm_swf_sound_output_storage_info](../../../include/flashvm/swf_sound_output.h#L74) | Observe output-owned bytes independently of borrowed mixer/FIFO |
| [fvm_swf_sound_output_submit_prepared_bounded](../../../include/flashvm/swf_sound_output.h#L114) | Transfer/clear native ownership or consume a queued NoMultiple duplicate, rechecking mixer bytes |
| [fvm_swf_sound_output_pump](../../../include/flashvm/swf_sound_output.h#L124) | Prepare once, retain rejected PCM and retry identical bytes before another composition |
| [fvm_swf_sound_mixer_info](../../../include/flashvm/swf_sound_mixer.h#L139) | Observe software retirement without independently advancing the borrowed mixer |
| [fvm_swf_sound_output_sound_active](../../../include/flashvm/swf_sound_output.h#L81) | Poll FIFO and compare exact exclusive ID endpoint with dequeue progress |
| [fvm_swf_sound_output_identity_info](../../../include/flashvm/swf_sound_output.h#L99) | Copy exact dequeue cursor/deadline for host diagnostics without source VM or allocation |
| [fvm_swf_sound_output_pause](../../../include/flashvm/swf_sound_output.h#L136) | Acknowledge model consumption policy without discarding PCM |
| [fvm_swf_sound_output_info](../../../include/flashvm/swf_sound_output.h#L138) | Copy exact pending versus accepted block accounting after write failure |
| [fvm_swf_sound_output_stop_id](../../../include/flashvm/swf_sound_output.h#L122) | Release ID and stop future composition while accepted PCM remains unchanged |
| [fvm_swf_sound_output_stop](../../../include/flashvm/swf_sound_output.h#L142) | Acknowledge pause/clear, cancel registrations and reset output epoch |
| [fvm_swf_sound_output_close](../../../include/flashvm/swf_sound_output.h#L153) | Release retry/ID storage after callbacks/readers stop; does not own mixer |
| [fvm_swf_sound_mixer_close](../../../include/flashvm/swf_sound_mixer.h#L143) | Release adopted native voices after output quiescence |
| [fvm_swf_sound_voice_close](../../../include/flashvm/swf_sound.h#L128) | Release only remaining untransferred owners, independently of the destroyed VM |

The checker compares every exported S16 value with an independent rational PCM
oracle, checks both identical blocks, exact report fields, replacement, usage,
invalid CLI and terminal output-path errors. Other codecs and real SDL behavior
are exercised separately by the native/player regression suite.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../../../include/flashvm/flashvm.h#L72) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_swf_sound_mixer_close` | function | [include/flashvm/swf_sound_mixer.h:143](../../../include/flashvm/swf_sound_mixer.h#L143) |
| `fvm_swf_sound_mixer_create_bounded` | function | [include/flashvm/swf_sound_mixer.h:65](../../../include/flashvm/swf_sound_mixer.h#L65) |
| `fvm_swf_sound_mixer_info` | function | [include/flashvm/swf_sound_mixer.h:139](../../../include/flashvm/swf_sound_mixer.h#L139) |
| `fvm_swf_sound_output_close` | function | [include/flashvm/swf_sound_output.h:153](../../../include/flashvm/swf_sound_output.h#L153) |
| `fvm_swf_sound_output_create` | function | [include/flashvm/swf_sound_output.h:50](../../../include/flashvm/swf_sound_output.h#L50) |
| `fvm_swf_sound_output_identity_info` | function | [include/flashvm/swf_sound_output.h:99](../../../include/flashvm/swf_sound_output.h#L99) |
| `fvm_swf_sound_output_info` | function | [include/flashvm/swf_sound_output.h:138](../../../include/flashvm/swf_sound_output.h#L138) |
| `fvm_swf_sound_output_pause` | function | [include/flashvm/swf_sound_output.h:136](../../../include/flashvm/swf_sound_output.h#L136) |
| `fvm_swf_sound_output_pump` | function | [include/flashvm/swf_sound_output.h:124](../../../include/flashvm/swf_sound_output.h#L124) |
| `fvm_swf_sound_output_set_identity_tracking` | function | [include/flashvm/swf_sound_output.h:61](../../../include/flashvm/swf_sound_output.h#L61) |
| `fvm_swf_sound_output_sound_active` | function | [include/flashvm/swf_sound_output.h:81](../../../include/flashvm/swf_sound_output.h#L81) |
| `fvm_swf_sound_output_stop` | function | [include/flashvm/swf_sound_output.h:142](../../../include/flashvm/swf_sound_output.h#L142) |
| `fvm_swf_sound_output_stop_id` | function | [include/flashvm/swf_sound_output.h:122](../../../include/flashvm/swf_sound_output.h#L122) |
| `fvm_swf_sound_output_storage_info` | function | [include/flashvm/swf_sound_output.h:74](../../../include/flashvm/swf_sound_output.h#L74) |
| `fvm_swf_sound_output_submit_prepared_bounded` | function | [include/flashvm/swf_sound_output.h:114](../../../include/flashvm/swf_sound_output.h#L114) |
| `fvm_swf_sound_voice_clone` | function | [include/flashvm/swf_sound.h:111](../../../include/flashvm/swf_sound.h#L111) |
| `fvm_swf_sound_voice_close` | function | [include/flashvm/swf_sound.h:128](../../../include/flashvm/swf_sound.h#L128) |
| `fvm_swf_sound_voice_open_bounded` | function | [include/flashvm/swf_sound.h:97](../../../include/flashvm/swf_sound.h#L97) |
| `fvm_swf_sound_voice_output_frames` | function | [include/flashvm/swf_sound_mixer.h:44](../../../include/flashvm/swf_sound_mixer.h#L44) |
<!-- source-links:end -->
