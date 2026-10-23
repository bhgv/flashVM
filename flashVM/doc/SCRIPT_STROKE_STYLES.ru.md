# Торцы и соединения ActionScript lineStyle

[MovieClip.lineStyle](../src/standard/movieclip.cpp#L1438) связывает поддержанные строки параметров Flash Player 8 с
той же геометрией штриха, которая используется восстановленными LINESTYLE2.
Поддержаны round, none (butt), square и round/bevel для сплошных, линейных,
радиальных и внутренних focal-градиентных линий. Исходная сигнатура описана в
[Adobe AS2 MovieClip.lineStyle](https://open-flash.github.io/mirrors/as2-language-reference/MovieClip.html#lineStyle()).
Реализация ограниченная; это не полная приёмка lineStyle или SWF <= 8.

## Аргументы и ошибки

| Аргумент | Реализованное поведение |
| --- | --- |
| thickness | Отсутствие или явный undefined отключает текущий штрих без преобразования остальных аргументов. Конечное число ограничивается 0..255; ноль означает hairline. Прежние дробные ширины сохраняются. |
| rgb | Отсутствие/undefined означает чёрный; конечное число преобразуется по прежнему правилу packed RGB24. |
| alpha | Отсутствие/undefined означает 100; конечное число ограничивается 0..100. |
| pixelHinting | Boolean-преобразование в SWF8/host; false/default поддержан, true даёт [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). |
| noScale | Строковое преобразование в SWF8/host; normal/default поддержан, остальные режимы дают [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). |
| capsStyle | Строковое преобразование в SWF8/host; поддержаны round/default, none и square. |
| jointStyle | Строковое преобразование в SWF8/host; поддержаны round/default и bevel; miter/неизвестное значение дают [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). |
| miterLimit | Для поддержанных соединений игнорируется без преобразования; miter отклоняется до использования аргумента. |

Загруженные SWF1-7 сохраняют прежнюю видимость основного метода и игнорируют
дополнительные аргументы без вызова их преобразований. Незагруженный host-контекст
использует последние поддержанные опции. Это не подтверждает точность видимости
метода до SWF6. Необязательный undefined сохраняет значения по умолчанию.
Другие опции сравниваются по полной длине преобразованной строки: суффикс после
NUL не считается частью корректного ключевого слова. Неподдержанная строка
возвращает ошибку, а не незаметный round. Неконечные thickness/color/alpha дают
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); ошибка преобразования сохраняет исходный статус.
Пограничные преобразования и неконечные значения требуют отдельной Flash-приёмки.

## Состояние, снимки и память

[line_options](../src/standard/movieclip.cpp#L1392) обрабатывает Boolean hinting и строки scale/cap/join после
числовых аргументов, в порядке параметров. Числа читает [line_number](../src/standard/movieclip.cpp#L1379), полные
строки сравнивает [line_option](../src/standard/movieclip.cpp#L1371). Поддержанные ветки не преобразуют miterLimit.
Пользовательские callbacks могут исполнить код и изменить состояние; эти явные
побочные эффекты не откатываются. Копия текущего состояния создаётся после
преобразований, сохраняя их изменения заливки и drawing state.

[line_style](../src/standard/movieclip.cpp#L1438) готовит временную карту свойств, возвращает отсутствующие cap/join
к round, удаляет только текущий lineGradient и сбрасывает active. Одна swap
публикует стиль после успешных выделений. Нативный отказ памяти, неподдержанная
опция или ошибка преобразования не заменяют стиль частично, кроме явных
побочных эффектов пользовательского кода. Завершённые paint и их градиентные
рёбра сохраняют независимость.

Новый drawing record копирует необязательные lineCap/lineJoin. Следующий
lineStyle начинает отдельную запись; clear удаляет selectors. beginFill/endFill
и lineGradientStyle сохраняют выбранный стиль линии в соответствующих ветках.
Round остаётся отсутствующим полем для совместимости старых графов. Публичный
C89 ABI, callback ID и wire-форматы не меняются. Рендеринг, геометрический hit
и прямолинейные bounds используют те же скаляры.

Нативный барьер сохраняет receiver, аргументы и граф во время преобразований.
Явная повторная сборка из нативного callback отклоняется прежней политикой GC;
после внешнего вызова области восстанавливаются и сборка выполняется успешно.
Binary/channel сохраняют текущий и завершённый стиль после удаления отправителя.
Рабочие потоки используют изолированные контексты.

## Пример и ограничения

Исполняемый [C89-пример](../examples/embedding/stroke_styles/README.ru.md)
проверяет границы и попадание в угол square, затем непрозрачный пиксель торца
при доступном AGG. В NO_GRAPHICS геометрические проверки выполняются.
Пример использует обычную обработку ошибок и явно удерживает объект результата.

Miter, pixel hinting, non-scaling, точные оболочки кривых, прежние round hairline
bounds, неравномерные/shear метрики Flash, полная независимая приёмка
coercion/растра, Cairo и реальные устройства остаются открытыми. Все девять
этапов плана и полная совместимость SWF <= 8 остаются открытыми.


## Fixtures

| Fixture | Проверка |
| --- | --- |
| [movieclip_script_stroke_styles](../tests/movieclip_script_stroke_styles.cpp) | Шесть сохранённых сплошных стилей и GC |
| [movieclip_script_stroke_defaults](../tests/movieclip_script_stroke_defaults.cpp) | Ограничение ширины, defaults и отключение |
| [movieclip_script_stroke_versions](../tests/movieclip_script_stroke_versions.cpp) | Параметры в SWF1-8 |
| [movieclip_script_stroke_errors](../tests/movieclip_script_stroke_errors.cpp) | Unsupported, неконечные значения и embedded NUL |
| [movieclip_script_stroke_transitions](../tests/movieclip_script_stroke_transitions.cpp) | Отдельные записи, clear и возврат round |
| [movieclip_script_stroke_gradients](../tests/movieclip_script_stroke_gradients.cpp) | 18 линейных/радиальных/focal стилей |
| [movieclip_script_stroke_curves](../tests/movieclip_script_stroke_curves.cpp) | 48 квадратичных и угловых путей |
| [movieclip_script_stroke_quality](../tests/movieclip_script_stroke_quality.cpp) | 96 сочетаний качества/стиля/краски |
| [movieclip_script_stroke_bounds](../tests/movieclip_script_stroke_bounds.cpp) | Выбранные bounds и shape hit |
| [movieclip_script_stroke_mixed](../tests/movieclip_script_stroke_mixed.cpp) | Независимость заливки/текущего стиля/пути |
| [movieclip_script_stroke_callbacks](../tests/movieclip_script_stroke_callbacks.cpp) | Порядок callbacks, ошибки и побочные эффекты fill |
| [movieclip_script_stroke_allocation](../tests/movieclip_script_stroke_allocation.cpp) | Шесть атомарных серий C++-отказов |
| [movieclip_script_stroke_duplicate](../tests/movieclip_script_stroke_duplicate.cpp) | Шесть независимых копий текущего/завершённого стиля |
| [movieclip_script_stroke_transfer](../tests/movieclip_script_stroke_transfer.cpp) | 24 графа через binary и канал после удаления отправителя |
| [movieclip_script_stroke_threads](../tests/movieclip_script_stroke_threads.cpp) | Четыре потока, 96 циклов render/GC |
| [movieclip_script_stroke_avm1](../tests/movieclip_script_stroke_avm1.cpp) | 18 реальных версионных AVM1 вызовов |
| [jit_script_stroke_styles](../tests/jit_script_stroke_styles.cpp) | 36 режимов/стилей со счётчиками нативных входов |
| [movieclip_script_stroke_timeline](../tests/movieclip_script_stroke_timeline.cpp) | Действия кадров и 16 чередующихся seek |

## Контрольная точка v0.1324

Параметры script lineStyle v0.1304–v0.1324: SWF8 и незагруженный host принимают round/none/square и round/bevel через существующий AS-метод. Текущий стиль публикуется атомарно после упорядоченных преобразований и выделений; новый путь копирует selectors, завершённый сохраняет стиль, clear возвращает defaults. Отсутствие/undefined thickness отключает штрих; конечная ширина ограничивается 0..255, undefined color/alpha сохраняют defaults. SWF1-7 игнорируют дополнительные аргументы без преобразований. Hinting, non-normal scale, miter и неизвестные опции явно неподдержаны без частичной публикации; неконечные width/color/alpha недопустимы. Восемнадцать fixtures проверяют сплошную/градиентную геометрию, шесть ширин и defaults/disable, версии, unsupported/NUL/неконечные значения, отдельные записи и сброс, 18 градиентных стилей, 48 кривых/углов, 96 качеств, bounds/hit, независимость заливки, шесть этапов ошибок callbacks с восстановлением GC-барьера и явными побочными эффектами fill, шесть серий отказов C++-памяти, шесть дубликатов, 24 графа через binary и канал после удаления отправителя, четыре OS-потока с 96 циклами render/GC, 18 реальных AVM1 случаев, 36 конфигураций интерпретатора/manual/native JIT со счётчиками нативных входов и действия кадров при 16 чередующихся seek. Полностью комментированный C89-пример проверяет bounds, square-only hit и доступный пиксель, явно удерживая объект результата. Пройдены 247 выбранных нативных тестов с примером, 18 SDK-free тестов без графики (17 fixtures и пример), 17 ASan/UBSan (16 fixtures и пример), allocation с явным --no-faults и проверки исходников/примеров. JIT подтверждён нативно; реальные C++-отказы проверены нативно и без графики. Sanitizer unwinding ограничен, LeakSanitizer отключён. Публичный C89 ABI, callback ID и wire-форматы сохраняются. Полная coercion/reference-приёмка, видимость до SWF6, miter/hinting/non-scaling, точные кривые/неравномерные метрики, Cairo и реальные платформы открыты. Все девять этапов открыты. [Контракт](SCRIPT_STROKE_STYLES.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
