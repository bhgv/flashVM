# Снимки глубин при отрисовке — v0.630

[`fvm_movieclip_render`](../include/flashvm/movieclip.h#L174) и [`fvm_movieclip_render_view`](../include/flashvm/movieclip.h#L179) синхронно рисуют выбранный
MovieClip в потоке владельца контекста. Они не двигают timeline, не опрашивают
таймеры и не посылают события ввода. Вызывающий сохраняет контекст, клип и доступную
для записи память [`FVM_RasterSurface`](../include/flashvm/raster.h#L16) на весь вызов. Вариант viewport копирует
конечную [`FVM_Affine`](../include/flashvm/path.h#L39) до getters и применяет её один раз к каждому узлу.
Тот же обход обслуживает источник MovieClip в BitmapData.draw и coverage.

После собственной геометрии узла копируются идентичности детей и ключи глубины.
Весь локальный снимок проверяется до первого getter ребёнка. Затем дети рекурсивно
рисуются по возрастанию знаковой глубины независимо от порядка добавления.
[`fvm_depth_order::sort`](../src/core/depth_order.hpp#L29) заменяет сортировку вставками на heapsort: O(n log n)
работы сортировки и O(n) временной памяти. Отключённый [`FVM_ExecutionWorkScope`](../src/core/execution_work_scope.hpp#L11)
не сбрасывает и не расходует instruction counter. Сами getters сохраняют обычные
правила исполнения и ошибок. Новый бюджет работы raster этим не вводится.

Если getter ребёнка заменил следующего на той же глубине или создал нового, новый
объект ждёт следующего локального снимка. Перестановка глубины сохраняет место
захваченного объекта до следующего вызова. Перед каждым ребёнком проверяются
физический parent и эффективное удаление через предков; отсоединённые пропускаются.
Явно выбранный удалённый корень сохраняет прежнюю политику прямой отрисовки,
его отсоединённые потомки пропускаются. Дети, созданные getters корня до снимка,
могут участвовать сразу. Единого атомарного снимка всего дерева нет.

Границы независимы от tick: [`FVM_MOVIECLIP_RENDER_MAX_DEPTH`](../src/standard/movieclip_constants.h#L35) разрешает 64 уровня
потомков, [`FVM_MOVIECLIP_RENDER_MAX_CHILDREN`](../src/standard/movieclip_constants.h#L36) — 65536 слотов детей одного узла.
До первого getter ребёнка отклоняются null, не-MovieClip, чужой owner, ссылка на
самого родителя и повторная идентичность. Null/чужие private paths/children/state
также недопустимы. Повреждения и превышение границ дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47),
отказ выделения — [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46). Ошибки getters передаются вызывающему;
геометрия без raster backend может вернуть [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48). Уже записанные
пиксели и эффекты getters сохраняются: операция не является транзакцией изображения
и не перезапускается автоматически.

[GC:] Внешний [`FVM_NativeCallGuard`](../src/core/native_call_guard.hpp#L11) сохраняет захваченные объекты при замене и
удалении; временные контейнеры освобождаются при возврате и исключении. Сырые
идентичности не переживают вызов. Сборка и уничтожение контекста подчиняются обычным
ограничениям native call. Это не конкурентный renderer и не полная совместимость
Flash display list, masks, filters, lifecycle или оригинального Player. Мутация
команд paths и общий бюджет геометрии остаются отдельными задачами.

`movieclip_render_snapshot` проверяет host/SWF5–8, обычный и viewport-вызовы,
замену, swapDepths, удаление предка и ошибку getter. `movieclip_render_invalid`
проверяет повреждения, алиасы и превышение локальной границы.
`movieclip_render_depth_pixels` проверяет перекрытие RGB, viewport и транспорт
графа после уничтожения исходного контекста и GC. `movieclip_render_allocation`
перебирает отказы выделения до полного успеха обхода пустой геометрии.
`depth_order` проверяет 65536 обратных ключей с явной границей n log n.
Пиксельные проверки требуют backend; безопасность пустой геометрии проверяется и
без него. Это Linux-проверки, а не приёмка устройств или других ОС.

Исполняемый [C89-пример](../examples/embedding/movieclip/README.ru.md) рисует
пустую сцену с перемешанными глубинами и проверяет отсутствие лишнего timeline
callback. [English contract](RASTER_DEPTH_ORDER.md). Все девять стадий открыты.

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
| `FVM_Affine` | struct | [include/flashvm/path.h:39](../include/flashvm/path.h#L39) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:11](../src/core/execution_work_scope.hpp#L11) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `fvm_movieclip_render` | function | [include/flashvm/movieclip.h:174](../include/flashvm/movieclip.h#L174) |
| `FVM_MOVIECLIP_RENDER_MAX_CHILDREN` | macro / define | [src/standard/movieclip_constants.h:36](../src/standard/movieclip_constants.h#L36) |
| `FVM_MOVIECLIP_RENDER_MAX_DEPTH` | macro / define | [src/standard/movieclip_constants.h:35](../src/standard/movieclip_constants.h#L35) |
| `fvm_movieclip_render_view` | function | [include/flashvm/movieclip.h:179](../include/flashvm/movieclip.h#L179) |
| `FVM_NativeCallGuard` | class | [src/core/native_call_guard.hpp:11](../src/core/native_call_guard.hpp#L11) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `FVM_RasterSurface` | struct | [include/flashvm/raster.h:16](../include/flashvm/raster.h#L16) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
