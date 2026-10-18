# Native StopSounds / Остановка звука из JIT — v0.556

## English

AVM1 StopSounds (0x09) now lowers to the direct [fvm_jit_stop_sounds](../src/jit/effects.cpp#L7) semantic
helper. [fvm_jit_decode](../src/jit/decode.cpp#L148) emits a normal source checkpoint followed by a fixed
helper call. It invokes [fvm_audio_stop_all](../include/flashvm/multimedia.h#L91) exactly as the interpreter does;
it does not dispatch bytecode from machine code. There are no operands, no
pushed result and no new public ABI. Lower stack values remain intact.

Empty registries and entirely inactive registrations succeed without output
callbacks. Active registrations are visited in registration order. Every active
output receives one stop attempt even if an earlier callback fails. Return the
first failure; successful channels become inactive and failed channels remain
active for a later explicit retry. Other contexts and unregistered standalone
outputs or [FVM_SWFSoundVoice](../src/swf/sound_voice.cpp#L9) snapshots are untouched. The action does not close
channel registrations, free userdata, resample/mix audio or advance MovieClip.
Actual playback/discard remains the host output's responsibility.

[GC:] [fvm_audio_stop_all](../include/flashvm/multimedia.h#L91) holds [FVM_AudioCallbackGuard](../src/multimedia/audio/channel.hpp#L19) across the complete stable
registry. GC collection, context destruction and nested channel mutation are
blocked during callbacks. Metadata queries can inspect previous acknowledged
active state. Host userdata remains owned by the host until channel/context
cleanup completes. Callbacks must obey the C ABI and return normally; no C++
exception or cross-thread access is permitted. The helper itself allocates no
memory. Compilation and ordinary function activation can still return
[FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44); delivered host side effects are never rolled back.

A host failure propagates through ordinary native unwinding. Existing opt-in
exception replay policy still applies to actual script exceptions: it is not a
transaction and must not be used to assume output-side-effect rollback. Default
execution performs no diagnostic replay. Source checkpoint, budget and debugger
handling use the shared JIT path before the stop callback.

## Русский

StopSounds (0x09) компилируется в прямой вызов [fvm_jit_stop_sounds](../src/jit/effects.cpp#L7) после обычного
source checkpoint. Helper вызывает [fvm_audio_stop_all](../include/flashvm/multimedia.h#L91), как интерпретатор; нет
вложенного bytecode dispatch, новых публичных функций, аргументов или результата
на стеке. Нижние операнды сохраняются.

Пустой registry и неактивные выходы успешны без callbacks. Для каждого active
выхода выполняется одна попытка stop в порядке регистрации, включая выходы
после первого отказа. Возвращается первая ошибка. Успешные каналы становятся
inactive; неудачные остаются active для явного повторения. Другие контексты,
незарегистрированные outputs и [FVM_SWFSoundVoice](../src/swf/sound_voice.cpp#L9) snapshots не затрагиваются.
Регистрации и userdata остаются живы; MovieClip clock и mixer не запускаются.

[GC:] [FVM_AudioCallbackGuard](../src/multimedia/audio/channel.hpp#L19) удерживает registry стабильным и запрещает сборку,
уничтожение context и повторные create/write/stop/close во время callback.
Запрос active разрешён и видит ранее подтверждённое состояние. Userdata —
собственность host до завершения cleanup. Callback выполняется синхронно на
VM thread, должен нормально вернуть C ABI status. Helper не выделяет память;
компиляция/активация могут завершиться [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44). Доставленный host stop
не откатывается при последующей ошибке. Opt-in diagnostic exception replay
также не является транзакцией; по умолчанию он отключён.

## Executable example / Исполняемый пример

[swf_mix](../examples/multimedia/swf_mix/README.md) prepares two independent
snapshots, mixes blocks into a real stdio FILE and stops it with a normal AVM1
function. The tutorial selects automatic JIT only if [fvm_jit_available](../include/flashvm/jit.h#L13) succeeds
and requires exactly one actual native entry using [fvm_execution_get_stats](../include/flashvm/execution_mode.h#L71).
Otherwise it runs the same action in interpreter mode. Optional tutorial
`--interpreter` / `--jit` arguments explicitly select the path; invalid or
unavailable selections fail before VM/file creation. INTERPRETER profiles select
the initial execution policy, not necessarily compiler unavailability. No audio SDK or device
is needed. See its README for exact setup, output, ownership and API links.

## Verification / Проверки

- `tests/jit_stop_sounds.cpp`: isolated host and SWF1–8 contexts, interpreter,
  explicit native attachment and automatic JIT; native-entry counters, unchanged
  lower operand, empty/inactive registries, ordered multiple failures, retry,
  context isolation, actual script-exception unwinding without implicit replay,
  before/after-action instruction-budget gates, real callback delivery and
  callback lifecycle barriers.
- `tests/audio_channels.c`: the existing C89 host bridge now checks actual
  native-entry increments for its automatic-JIT StopSounds function.
- `tests/jit_stop_sounds_allocation.cpp`: invocation allocation-failure boundaries,
  restored roots/depths/active-code state, acknowledged host-side effects, and
  allocation-denied direct helper execution in host and loaded contexts.
- `scripts/check_swf_mix_example.py --native` or `--interpreter`: actual C89
  tutorial output and selected dispatch path, exact mixed bytes, overwrite,
  empty-argument behavior and failed native file creation cleanup. `--ffmpeg`
  adds the independent installed FFmpeg amix oracle.

These checks do not establish physical audible playback, complete sound/timeline
support, or foreign platform acceptance. No roadmap stage is closed.
Эти проверки не закрывают этапы 3/4/8/9 и не заменяют native platform acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_stop_all` | function | [include/flashvm/multimedia.h:91](../include/flashvm/multimedia.h#L91) |
| `FVM_AudioCallbackGuard` | class | [src/multimedia/audio/channel.hpp:19](../src/multimedia/audio/channel.hpp#L19) |
| `fvm_execution_get_stats` | function | [include/flashvm/execution_mode.h:71](../include/flashvm/execution_mode.h#L71) |
| `fvm_jit_available` | function | [include/flashvm/jit.h:13](../include/flashvm/jit.h#L13) |
| `fvm_jit_decode` | function | [src/jit/decode.cpp:148](../src/jit/decode.cpp#L148) |
| `fvm_jit_stop_sounds` | function | [src/jit/effects.cpp:7](../src/jit/effects.cpp#L7) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_SWFSoundVoice` | struct | [src/swf/sound_voice.cpp:9](../src/swf/sound_voice.cpp#L9) |
<!-- source-links:end -->
