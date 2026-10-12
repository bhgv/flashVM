# MovieClip.attachBitmap / Присоединение bitmap

attachBitmap(BitmapData, depth, pixelSnapping, smoothing) создаёт скрытый
статический display node с живой ссылкой на BitmapData. У него нет имени в
свойствах родителя; getInstanceAtDepth возвращает undefined, но узел участвует
в порядке глубин, getNextHighestDepth, отрисовке и bounds родителя. clear удаляет
нарисованные paths родителя и не удаляет присоединённое изображение.

Параметры задаются при присоединении; повторный attachBitmap заменяет узел
на той же глубине. Новый узел и прежнее состояние отсоединения подготавливаются
до публикации: OOM сохраняет прежнюю запись и её parent edge. Источник не
копируется, setPixel меняет отрисовку. Dispose источника прекращает bitmap draw,
сохраняя display node и геометрию; точные Flash bounds после dispose ещё открыты.

pixelSnapping по умолчанию auto: translation округляется при identity linear
matrix, без растяжения/вращения. always округляет translation при любой матрице;
never оставляет дробные координаты. Snapping влияет только на render-world
matrix, не меняя script-visible x/y/scale. Матрицы отражений, масштабирования,
вращения и наследуемые цвета используют общий bitmap sampler; smoothing по
умолчанию false. Точный Flash threshold auto и quality/downsampling ещё открыты.

Проверяемые сценарии movieclip_attach_bitmap и allocation-версии: скрытая
идентичность, замена/глубины, clear, pixel snapping, live writes/dispose,
queue/file aliases, GC, импорт метода в контекст без стандартных globals,
отказы каждого new/new[] до успеха и баланс отслеживаемых C++ allocations.
Graph transport сохраняет bitmap_attachment scalar и общую source identity
в пределах посылки, создавая независимый receiver pixel buffer.

Остальные ограничения этапов 3–4 сохраняются: bitmap-aware hit testing, полная
реконструкция при timeline/depth collisions, duplicate attachments, SWF bitmap
resources, draw/фильтры, полный текст и медиа. Полное завершение не заявляется.

## English

attachBitmap creates a hidden static display node retaining the original live
BitmapData. It participates in depth ordering and parent bounds/rendering, but
is not a named/script-visible MovieClip. Parent clear leaves it attached.
Replacement prepares the node and old detachment maps before publication,
preserving the previous graph on OOM. Bitmap writes remain visible; disposal
suppresses painting without removing the node.

Auto/always/never position snapping operates on the render-only world translation;
auto currently requires an identity linear matrix. Script transforms remain
unchanged. The existing bitmap sampler supplies smoothing and inherited color
transforms. The named suites verify identity, depth replacement, snapping,
transport, GC and allocation failure/balance. Full Flash thresholds, disposed
bounds, bitmap hit testing, timeline collisions and duplicate attachment semantics
remain open, along with the rest of roadmap stage 4.
