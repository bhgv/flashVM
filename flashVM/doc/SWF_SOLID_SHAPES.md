# Solid SWF shapes / Сплошные векторные фигуры SWF

Development checkpoint v0.29. Roadmap stages 3 and 4 remain open.

DefineShape, DefineShape2 and DefineShape3 now decode solid RGB/RGBA fills,
legacy line styles, straight and quadratic edges, moves and new style banks.
Exact integer twip endpoints are retained while assembling directed fill contours;
commands convert to pixels after joining. Fill0/Fill1 select opposite sides, equal
styles cancel an internal boundary, and even-odd fills retain holes. Stroke paths
retain record order and explicit move/style breaks, including coincident endpoints.
Quadratic control points remain editable vector data rather than a flattened image.
The common path implementation computes Bezier extrema for bounds.

Static geometry uses internal display nodes with no MovieClip prototype, instance
name binding or script frame execution. getInstanceAtDepth does not expose such
nodes as MovieClips, and attachMovie does not instantiate exported shapes. They
participate in rendering, parent bounds, placement matrices/color and removal.
Replace changes a static shape's source art in place; other instances sharing the
previous immutable geometry remain unchanged. A MovieClip's Replace semantics
remain different, as documented in SWF_SCENE_CONTROLS.md.

Definitions, paints and vector commands are ordinary managed graph records;
there are no Cairo/AGG pointers or borrowed input ranges in transported data.
Queue, file and snapshot transport retain contours and resource sharing. Native
construction barriers protect temporary edges until adoption; comments use
the `[GC:]` prefix. Loading runs no script and leaves the output null on failure.
The native shape decoder publishes output only after validating the full record.

Bounds, IDs, indices and record boundaries are checked. Coordinates are limited
to +/-1,000,000 pixels, accumulated style banks to 4096 fills/4096 lines, edges
to 65536 and shape records to 131072. Limit exhaustion returns OUT_OF_MEMORY.
AGG/Cairo drawing retains the existing stricter raster coordinate/surface limits.
Zero-width strokes render as a one-pixel hairline rather than becoming a fill.
The decoder uses C++98 and project-owned containers; public APIs remain C89.

## Реализация и проверки

Добавлено чтение DefineShape/2/3: сплошные RGB/RGBA-заливки, старые стили линий,
прямые и квадратичные рёбра, перемещения и новые банки стилей. Контуры собираются
по точным координатам в твипах, затем переводятся в пиксели. Заливка учитывает
отверстия; обводка сохраняет порядок рёбер и явные разрывы. Контрольные точки
кривых остаются векторными данными; размеры вычисляются по экстремумам кривых.

Статическая фигура является внутренним узлом display list, без прототипа
MovieClip, привязки имени и исполнения кадрового сценария. getInstanceAtDepth и
attachMovie не выдают её за MovieClip. Она участвует в отрисовке, размерах
родителя, преобразованиях и удалении. Replace меняет её геометрию, сохраняя узел;
другие экземпляры прежнего ресурса сохраняют исходную геометрию.

Ресурсы и команды входят в управляемый граф и переносятся через очередь, файл
и снимок, включая общие ссылки. Загрузка не исполняет сценарии; ошибки оставляют
нулевой результат. Нативный декодер публикует результат только после полной
проверки записи. Лимиты приведены выше; ограничения растеризации строже.

`tests/swf_shape.cpp` checks the three tags, holes, quadratic extrema, new banks,
static-node semantics, Replace/seek, resource sharing, queue/file/snapshot transport,
GC, every truncated prefix, trailing bytes and dictionary-ID collisions. Pixel
assertions run against both bundled raster backends. `tests/swf_shape_allocation.cpp`
injects each allocation boundary during native decoding, graph loading and first
placement; it checks unchanged native output/null importer output and collection
after failure. Existing MovieClip/timeline/transport tests remain part of acceptance.

## Remaining / Остаётся

Gradient and bitmap fills, DefineShape4/LINESTYLE2, morph shapes, masks, filters,
blend modes, text/button/bitmap resources, audio/video and full historical-player
comparison are still open. Unsupported style/tag variants are rejected explicitly.
Line scaling, joins/caps across all transforms, detailed depth/reference semantics,
all SWF-version differences and aggregate memory budgets need further acceptance.
NO_GRAPHICS validates geometry/transport but cannot confirm rendered pixels.
Non-Linux platform runs and complete Flash compatibility are not certified.

Градиенты, bitmap-заливки, DefineShape4, морфинг, маски, фильтры, смешивание,
остальные ресурсы и мультимедиа ещё не завершены. Нужна полная сверка с эталонным
Flash Player, правил линий/масштабирования, глубин, версий и общего лимита памяти.
Эта контрольная точка не закрывает этапы дорожной карты.

Reference: [Adobe SWF specification, Shape Records and DefineShape](https://open-flash.github.io/mirrors/swf-spec-19.pdf).

## Validation v0.29 / Проверки v0.29

Strict Linux C89/C++98 profiles passed: headless 179/179, bundled AGG 189/189,
bundled Cairo 189/189 and portable/no-OS 170/170. Three portable programs needed
executable-permission repairs and successful reruns; initial and retry logs are
preserved separately. The sanitizer driver completed 176 tests/examples with
heap/global ASan and UBSan. ASan stack checks were explicitly disabled for the
workspace unwind environment; LeakSanitizer was disabled because /proc is
unavailable; container exception tests use UBSan only. The source comment audit
covers 240 files with no missing function/member/tutorial comments; the direct
GC audit found no missing tagged comments. These are checkpoint results, not full
Flash or target-platform certification.
