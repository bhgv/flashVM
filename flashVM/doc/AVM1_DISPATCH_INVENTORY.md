# AVM1 source dispatch inventory / Реестр диспетчеров — v0.329

The factual [catalog](AVM1_DISPATCH_CATALOG.csv) records the 100 byte/name pairs
from Ruffle's pinned [AVM1 opcode enum](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/swf/src/avm1/opcode.rs).
It is a bounded comparison catalog, not an assertion that every reserved,
deprecated or vendor-specific Flash action is represented by that enum.

Run `python3 scripts/check_avm1_dispatch.py` from the source checkout; redirect
its generated JSON outside Git. The audit validates catalog duplicates/byte
ranges, finds documented Action constants and checks their source references in
the interpreter's execution loop after removing comments and quoted literals.
`--require-all` now passes: all 100 entries have source references as of v0.546.
StopSounds (0x09) stops active context-registered host outputs through the shared
[AUDIO_CHANNELS.md](AUDIO_CHANNELS.md) operation. This does not implement SWF
DefineSound/StartSound/streaming audio or certify complete sound playback.

Since v0.556 StopSounds also has direct native lowering with ordered host
callback/error semantics and actual-entry checks: [JIT_SOUND.md](JIT_SOUND.md).
С v0.556 StopSounds проверен также в explicit/automatic JIT; source inventory
по-прежнему не доказывает полноту AVM1.

A source reference certifies neither operand decoding nor runtime semantics,
SWF version conformance, GC safety or historical player equivalence. Selected
behavioral evidence remains in COMPATIBILITY_STATUS.md and the service reports.
Completing this dispatch gate alone cannot close stage 2 or any later stage.

Каталог проверяемый: отсутствие ветки отображается явно и не заменяется отметкой
о совместимости. Ветки TargetPath, RandomNumber, GetTime и обе проверки кадров присутствуют,
RemoveSprite подключён для описанных в AVM1_REMOVE_SPRITE.md нативных сценариев;
CloneSprite подключён через общий C-сервис (AVM1_CLONE_SPRITE.md);
StartDrag/EndDrag подключены к общему C-механизму (AVM1_DRAG.md);
ToggleQuality подключён к состоянию сцены и LOW/HIGH vector coverage
([STAGE_QUALITY.md](STAGE_QUALITY.md)); StopSounds подключён к явно
зарегистрированным аудиовыходам контекста ([AUDIO_CHANNELS.md](AUDIO_CHANNELS.md)). Полная приёмка требует
реальных SWF, проверок версий, исключений, GC и всех остальных пунктов роудмапа.
Сгенерированный JSON в исходники и историю Git не включается.
