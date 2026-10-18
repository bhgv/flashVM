# Classic display properties / Классические свойства сцены

v0.113 adds ActionGetProperty (0x22) and ActionSetProperty (0x23). Numeric
index coercion precedes target resolution, then writable value coercion. Direct
MovieClip arguments bypass string conversion; other arguments resolve counted
paths relative to the current target or activation base after invalid selection.
An empty path selects only a valid current target. Invalid indices/targets give
undefined on reads and no change on writes. Invalid/read-only writes do not
coerce their value under the current Flash Player 7/8 policy; later player
behavior and selectable profiles remain open.

Implemented/tested indices: 0 _x, 1 _y, 2 _xscale, 3 _yscale, 4 _currentframe,
5 _totalframes, 6 _alpha, 7 _visible, 10 _rotation, 11 _target, 12 _framesloaded,
13 _name. v0.327 also supports 20 _xmouse and 21 _ymouse through the shared
clip-local pointer query (CLIP_POINTER.md). v0.379 adds indexed vector dimension
reads 8/9 (MOVIECLIP_INDEXED_SIZE.md); v0.385 adds vector dimension writes
(MOVIECLIP_DIMENSION_WRITES.md). Quality indices 16/19 now use the shared context policy ([quality contract](STAGE_QUALITY.md#script-quality-properties)). Remaining system/media
fields 14/15/17/18 explicitly return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) for a valid target. Pointer reads
project the context position through the inverse world matrix; other scalar reads use stored intrinsic fields;
frame counts and slash target use native state. Visibility reads a boolean.
Position writes truncate to twips with Int32 saturation; coordinate infinities
select the minimum coordinate. Rotation is normalized into [-180,180]. Null,
undefined and numeric NaN are ignored by geometry numeric setters; quality has
its own versioned conversion policy. Non-coordinate
infinities remain unsupported. Name writes use AVM1 string coercion, with a
numeric NaN name becoming "0" in SWF 7–8. Full rename binding reconciliation,
public named-member intrinsic behavior and complete matrix/field semantics are
not certified by these indexed actions.

[GC:] The resolved clip and converted values have temporary roots across later
script coercion. A regression detaches the selected child during valueOf,
actually collects, then safely publishes its scalar through the request root.

[`fvm_avm1_get_target_info`](../include/flashvm/avm1.h#L31) exposes struct-size-checked borrowed base/current
clips and active state to C89 hosts, including URL/trace callbacks. Invalid
arguments leave output unchanged. Outside activation the fields are inactive/
null. Hosts must retain borrowed clips before using them after scope exit or
later collection. This read-only API performs no allocation or script entry.

Добавлены индексные GetProperty/SetProperty и просмотр target из C callback.
Проверены порядок valueOf/toString, различия SWF 4–8, twip-позиции, NUL,
readonly/no-op поведение и сохранение отсоединённого клипа при настоящем GC.
В v0.327 добавлены 20 _xmouse и 21 _ymouse через общий C-сервис
локальных координат (CLIP_POINTER.md). В v0.379 добавлено чтение векторных
размеров 8/9 (MOVIECLIP_INDEXED_SIZE.ru.md), в v0.385 — индексная запись
(MOVIECLIP_DIMENSION_WRITES.ru.md). Системные индексы
16/19 реализованы через общее качество контекста ([контракт](STAGE_QUALITY.md#свойства-качества));
14/15/17/18 ещё явно не поддерживаются. Полный named-member
контракт и согласование имени с родительскими bindings остаются открытыми.

avm1_display_properties and C89 avm1_target_api are the new tests. All 276 core
AGG/Cairo tests and the optional AGG runner test pass after five environment
execute-bit failures were restored and rerun. Four focused tests pass in each
portable and restricted ASan/UBSan profile. Ten original external SWF probes
still complete and match exactly. No roadmap stage is closed.
Primary cross-check: [pinned Ruffle intrinsic property map](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/stage_object.rs).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_avm1_get_target_info` | function | [include/flashvm/avm1.h:31](../include/flashvm/avm1.h#L31) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
