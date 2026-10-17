# AVM1 source dispatch inventory / Реестр диспетчеров — v0.329

The factual [catalog](AVM1_DISPATCH_CATALOG.csv) records the 100 byte/name pairs
from Ruffle's pinned [AVM1 opcode enum](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/swf/src/avm1/opcode.rs).
It is a bounded comparison catalog, not an assertion that every reserved,
deprecated or vendor-specific Flash action is represented by that enum.

Run `python3 scripts/check_avm1_dispatch.py` from the source checkout; redirect
its generated JSON outside Git. The audit validates catalog duplicates/byte
ranges, finds documented Action constants and checks their source references in
the interpreter's execution loop after removing comments and quoted literals.
`--require-all` fails if a catalog entry has no dispatch reference. It currently
fails as expected: 99 entries have references and StopSounds remains absent.

| Byte | Missing action | Area requiring implementation |
| --- | --- | --- |
| 0x09 | StopSounds | Audio playback integration |

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
([STAGE_QUALITY.md](STAGE_QUALITY.md)); StopSounds ещё отсутствует. Полная приёмка требует
реальных SWF, проверок версий, исключений, GC и всех остальных пунктов роудмапа.
Сгенерированный JSON в исходники и историю Git не включается.
