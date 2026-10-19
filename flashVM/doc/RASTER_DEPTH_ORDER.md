# Raster depth snapshots — v0.630

[`fvm_movieclip_render`](../include/flashvm/movieclip.h#L76) and [`fvm_movieclip_render_view`](../include/flashvm/movieclip.h#L79) draw the selected MovieClip
synchronously on its context owner thread. They do not advance timelines, poll timers
or dispatch input. The caller retains the context, selected clip and writable
[`FVM_RasterSurface`](../include/flashvm/raster.h#L16) storage for the entire call. The viewport variant copies a finite
[`FVM_Affine`](../include/flashvm/path.h#L33) before script getters and applies it once to each world-space node.
The same child traversal serves BitmapData.draw's MovieClip source and coverage pass.

After drawing a node's own geometry, capture that node's child identities and depth
keys. Validate the complete local snapshot before invoking any child getter. Draw
children in ascending signed public depth, recursively, independent of insertion
order. [`fvm_depth_order::sort`](../src/core/depth_order.hpp#L29) now replaces raster insertion sorting with heapsort:
O(n log n) sorting and O(n) scratch. It uses a disabled [`FVM_ExecutionWorkScope`](../src/core/execution_work_scope.hpp#L9), so
sorting neither resets nor consumes the enclosing instruction counter. Script getters
retain their ordinary execution/error behavior. This is not a new raster work budget.

A child getter may change the scene. A same-depth replacement/newborn waits for the
next local capture; a moved child keeps its captured position during this call, then
uses its new depth next time. Before drawing each captured child, check its physical
parent and effective removal through ancestors. Skip detached objects. An explicitly
selected removed root retains its existing direct-render policy; detached descendants
are skipped. Root getters run before the child snapshot, so children they create can
participate immediately. There is no whole-tree atomic snapshot.

Bounds are independent from tick bounds: [`FVM_MOVIECLIP_RENDER_MAX_DEPTH`](../src/standard/movieclip_constants.h#L30) permits
64 descendant levels; [`FVM_MOVIECLIP_RENDER_MAX_CHILDREN`](../src/standard/movieclip_constants.h#L31) permits 65536 child slots
per node. Reject null, non-MovieClip, foreign-owner, self-child and duplicate-identity
slots before the first child getter. Null/foreign private paths/children/state edges
are also invalid. [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) reports malformed graphs or bound violations;
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) reports allocation failure. Getter errors propagate and geometry
may return [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46) without a raster backend. Earlier pixels and getter
side effects remain committed: failure is not an image transaction or automatic retry.

[GC:] The enclosing [`FVM_NativeCallGuard`](../src/core/native_call_guard.hpp#L9) keeps captured native identities alive
through removal/replacement; scratch containers release on every return/exception.
No raw identity is retained beyond the call. Context teardown/collection must follow
ordinary native-call restrictions. This is not a concurrent renderer or a claim of
complete Flash display-list, masks, filters, lifecycle or original-player conformance.
Path-command mutation and geometry-wide resource budgeting remain separate work.

Tests: `movieclip_render_snapshot` covers host/SWF5–8 and ordinary/viewport routes,
replacement, same-parent depth moves, ancestor removal and getter errors;
`movieclip_render_invalid` covers private edges, aliases and oversize local tables;
`movieclip_render_depth_pixels` checks overlapping RGB geometry, viewport and graph
transport after source-context destruction and GC; `movieclip_render_allocation`
walks every allocation boundary until a complete empty-geometry traversal succeeds.
`depth_order` also checks 65536 reverse keys against an explicit n log n work bound.
Pixel checks require an enabled backend; empty-geometry safety tests run without one.
The tests are Linux evidence, not device/foreign-OS acceptance.

The runnable [C89 tutorial](../examples/embedding/movieclip/README.md) now renders
its empty shuffled-depth scene and verifies no extra timeline callback occurs.
[Russian contract](RASTER_DEPTH_ORDER.ru.md). All nine roadmap stages remain open.

Validation / Проверки v0.630: 183 selected Linux Release CTests passed with AGG
and bundled static dependencies; 125 minimal whole-core ASan/UBSan CTests passed
serially (LeakSanitizer disabled, raster backend absent). Actual pixel acceptance
comes from the Release run. Four new raster regressions and the extended shared
sort oracle passed. The tutorial compiled with strict C89/pedantic-errors, linked
to the sanitizer core and printed the documented four lines. All documentation
source-link and debugger-example checks passed. A source-linked control using the
exact v0.629 movieclip.cpp fails the replacement-delivery assertion; the current
implementation passes. This compares FlashVM implementations, not original Player.

Проверены Release-пиксели и отдельная минимальная ASan/UBSan-сборка; отключение
LeakSanitizer и отсутствие raster backend в ней не выдаются за полную проверку
утечек или ASan-пикселей. Исходный контроль v0.629 воспроизводит преждевременную
доставку getter заменённого ребёнка. Устройства и другие ОС не проверялись.

Reproduce from the independent `flashVM/` component root / Из каталога компонента:

```sh
cmake -S . -B build-raster -DFVM_DEFS_FILE=cmake/profiles/agg.defs -DFVM_ENABLE_RENDER=ON -DFVM_BUILD_EXAMPLES=ON
cmake --build build-raster --target flashvm_tests_depth_order flashvm_tests_movieclip_render_snapshot flashvm_tests_movieclip_render_invalid flashvm_tests_movieclip_render_depth_pixels flashvm_tests_movieclip_render_allocation flashvm_examples_embedding_movieclip_main
ctest --test-dir build-raster -R '^(flashvm_tests_(depth_order|movieclip_render_.*)|flashvm_examples_embedding_movieclip_main)$' --output-on-failure
```

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Affine` | struct | [include/flashvm/path.h:33](../include/flashvm/path.h#L33) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_render` | function | [include/flashvm/movieclip.h:76](../include/flashvm/movieclip.h#L76) |
| `FVM_MOVIECLIP_RENDER_MAX_CHILDREN` | macro / define | [src/standard/movieclip_constants.h:31](../src/standard/movieclip_constants.h#L31) |
| `FVM_MOVIECLIP_RENDER_MAX_DEPTH` | macro / define | [src/standard/movieclip_constants.h:30](../src/standard/movieclip_constants.h#L30) |
| `fvm_movieclip_render_view` | function | [include/flashvm/movieclip.h:79](../include/flashvm/movieclip.h#L79) |
| `FVM_NativeCallGuard` | class | [src/core/native_call_guard.hpp:9](../src/core/native_call_guard.hpp#L9) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_RasterSurface` | struct | [include/flashvm/raster.h:16](../include/flashvm/raster.h#L16) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
