# Торцы и соединения линии из C89

Диагональное дополнение: none/bevel, width=10, start (2,2), control (8,6.5), end (14,11). Длина отрезка 15, нормальные полуширинные смещения 3 и 4; проверяются bounds -1..17, -2..15 даже при moveTo(100,100). Новая строка: `linear curve bounds: -1..17, -2..15`. Владение и GC-корни заменяются как в предыдущих этапах. [Контракт](../../../doc/LINEAR_CURVE_BOUNDS.ru.md).

Дополнение осевой кривой: после round-примера задаются none/bevel, control (8,8), end (14,8) и одиночный moveTo(100,100). Проверяются bounds x=2..14, y=6..10. Дополнительная строка: `axis curve bounds: 2..14, 6..10`. Каждый заменяемый результат getBounds отдельно удерживается и освобождается. Публичные API и синхронное владение те же; новые функции C не требуются. [Контракт](../../../doc/AXIS_CURVE_BOUNDS.ru.md).

Дополнение round-кривой: после первого render пример удаляет старую линию, задаёт round/round, вызывает curveTo с control (8,0), end (14,8) и добавляет одиночный moveTo(100,100). Проверяется getBounds x=0..16, y=2..10. Старый bounds-корень снимается до замены пути; новый результат отдельно удерживается и освобождается при cleanup. clear, curveTo и moveTo синхронно меняют принадлежащую контексту геометрию через существующий [fvm_object_call](../../../include/flashvm/value.h#L126). Дополнительная строка вывода: `round curve bounds: 0..16, 2..10`. Растер самой кривой этим дополнением не проверяется. [Контракт](../../../doc/ROUND_CURVE_BOUNDS.ru.md).

[English](README.md). Пример вызывает script lineStyle с square/bevel,
рисует линию (2,8)..(14,8), проверяет bounds 0..16 и попадание в угол (0.1,6.1).
При доступном AGG дополнительно проверяет альфу 255 пикселя (0,6).
Он не требует SWF-компилятора, устройства, SDL или event loop.

Контракт параметров и ограничения: [SCRIPT_STROKE_STYLES.ru.md](../../../doc/SCRIPT_STROKE_STYLES.ru.md).

## Владение и выполнение

Контекст владеет clip и временным bounds; host отдельно pin-ит оба через VM-вызовы.
Числа копируются, строковые литералы заимствуются только на время синхронного
вызова. Буфер RGBA принадлежит стеку host и передаётся render без сохранения.
Cleanup снимает только полученные корни, затем удаляет контекст и runtime.
Ошибки дают ненулевой exit status без assertions. Main объясняет каждую строку.

## Сборка и запуск

Из отдельного компонента FlashVM:

```sh
cmake -S . -B build-stroke-styles -DFVM_DEFS_FILE="$PWD/cmake/profiles/text-jit.defs"
cmake --build build-stroke-styles --target flashvm_examples_embedding_stroke_styles_main
./build-stroke-styles/flashvm_examples_embedding_stroke_styles_main
```

```text
bounds: 0..16; square corner: hit
square corner alpha: 255
```

С `cmake/profiles/minimal.defs` используйте отдельный каталог сборки: вместо второй
строки выводится `renderer unavailable; geometry verified`, exit status остаётся 0.
Незагруженный host-контекст включает последние поддержанные опции; в SWF1-7
дополнительные аргументы игнорируются. Miter, hinting, non-scaling, Cairo,
неравномерные преобразования и полная Flash-приёмка этим примером не подтверждаются.

## Использованные C API

| Функция | Назначение, владение и выполнение |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L62) | Инициализировать конфигурацию вызывающей стороны; указатели не сохраняются. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L64) | Синхронно создать runtime; уничтожить после всех контекстов. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L68) | Создать отдельный контекст; объекты принадлежат ему. |
| [fvm_standard_install](../../../include/flashvm/standard.h#L20) | Синхронно установить встроенные script-методы этого контекста. |
| [fvm_movieclip_create](../../../include/flashvm/movieclip.h#L95) | Создать clip контекста; удержать перед следующими VM-вызовами. |
| [fvm_gc_pin](../../../include/flashvm/gc.h#L48) | Получить один host GC-корень для заимствованного clip или bounds. |
| [fvm_value_undefined](../../../include/flashvm/value.h#L40) | Инициализировать/сбросить значение вызывающей стороны без выделения. |
| [fvm_value_number](../../../include/flashvm/value.h#L42) | Скопировать число в память вызывающей стороны. |
| [fvm_value_boolean](../../../include/flashvm/value.h#L44) | Скопировать Boolean без удержания объектов. |
| [fvm_value_string](../../../include/flashvm/value.h#L46) | Заимствовать статическую строку на время синхронного вызова. |
| [fvm_object_call](../../../include/flashvm/value.h#L126) | Синхронно вызвать lineStyle/drawing/hitTest/getBounds; аргументы заимствованы, объекты результата принадлежат контексту. |
| [fvm_object_get](../../../include/flashvm/value.h#L72) | Прочитать числовое свойство удержанного bounds; getters могут исполняться. |
| [fvm_movieclip_render](../../../include/flashvm/movieclip.h#L135) | Синхронно заимствовать host surface; без renderer вернуть UNSUPPORTED. |
| [fvm_gc_unpin](../../../include/flashvm/gc.h#L54) | Освободить ровно один полученный корень; не читать объект после удаления контекста. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L70) | Удалить объекты контекста после снятия host-корней. |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L66) | Освободить runtime только после контекста. |

[main.c](main.c) · [Catalog](../../README.md#example-embedding-stroke_styles).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_movieclip_create` | function | [include/flashvm/movieclip.h:95](../../../include/flashvm/movieclip.h#L95) |
| `fvm_movieclip_render` | function | [include/flashvm/movieclip.h:135](../../../include/flashvm/movieclip.h#L135) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../../../include/flashvm/value.h#L126) |
| `fvm_object_get` | function | [include/flashvm/value.h:72](../../../include/flashvm/value.h#L72) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
| `fvm_value_boolean` | function | [include/flashvm/value.h:44](../../../include/flashvm/value.h#L44) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
| `fvm_value_string` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../../../include/flashvm/value.h#L40) |
<!-- source-links:end -->
