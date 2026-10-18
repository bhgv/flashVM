# Root-relative hit points / Координаты hitTest относительно корня — v0.318

Loaded numeric MovieClip.hitTest now interprets point coordinates in the original
unlocked AVM1 root's coordinate system. The point is transformed to world space
using that root's affine matrix before comparison with native world bounds or
paint geometry. A translated/scaled root therefore gives the same logical point
results. _lockroot on the tested clip does not change this coordinate system.
The unlocked-root conversion lives in its own movieclip_hit_point.cpp function.

For the loaded point form, numeric arguments and the optional shape flag are
converted before geometry is evaluated. A valueOf that changes root position is
observed by the subsequent geometry query. Nonfinite input points return false.
The shape flag now uses the existing versioned Boolean C service: pre-SWF7 string
truth is numeric, while SWF7+ uses string length. Objects do not execute extra
truth-conversion callbacks. Explicit macros describe argument roles and the
unlocked ancestry walk bound.

Unloaded host calls retain world-coordinate and earlier shape-flag policy.
Object-to-object hitTest behavior is unchanged. The receiver is validated before
point coercion, preserving rejection of borrowed nonclip receivers. Existing
native bounds/shape limits and native collector barriers remain in force.
The helper publishes both transformed coordinates only after root/matrix lookup
succeeds; method errors restore native/root/script call scopes.

movieclip_hit_root_coordinates is C89 and covers host/SWF4–8, root translation and
scale, triangle bounds vs fill, lock independence and shape-string truth.
movieclip_hit_root_callbacks moves the root inside numeric conversion and also
throws; it checks one conversion and restored call scopes in both shape modes.
movieclip_hit_root_allocation injects every preparation failure for bounds and
shape tests, collecting between attempts. Native callbacks retain their existing
collector barrier; collection inside arbitrary native geometry getters is not
claimed supported by these fixtures.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/globals/movie_clip.rs hit_test converts numeric arguments first and
maps its point through avm1_root_no_lock().local_to_global before bounds/shape
queries. Complete single-target/path overloads, missing argument rules, detached
geometry, strokes/filters and the entire historical hitTest API are not certified.
All nine roadmap stages remain open.

В загруженных SWF точка hitTest теперь преобразуется из координат исходного
корня в мировые координаты; `_lockroot` на это не влияет. Преобразования аргументов
выполняются до чтения геометрии, поэтому изменение положения корня внутри valueOf
учитывается. Проверены host/SWF4–8, границы и заливка треугольника, масштаб и сдвиг
корня, версии строкового флага формы, исключения и отказы памяти. Прежний host-режим
и объектная перегрузка не менялись. Полная геометрия и стадии роудмапа ещё открыты.

Validation: GCC 13.3 Release AGG/Cairo each passed 619 tests directly after the
final receiver-validation change. Three targeted heap/global ASan+UBSan fixtures
passed (no stack/leak instrumentation). The four recovered original assets from
v0.317 matched exact pinned output with empty stderr in each profile using
--acknowledge-quit --timeline 1. The historical 26-asset corpus was not rerun after
workspace pruning. The bounded Action-reference audit remains 95/100.
