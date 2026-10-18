# Геометрия / Geometry

example_as2.as показывает создание Point/Matrix/Rectangle, перенос координат,
проверку полуоткрытых границ, Point-свойства и увеличение области.
Каждая строка объяснена. AS2 пример предназначен для внешнего компилятора,
который выдаёт AVM1 SWF; FlashVM пока не компилирует исходный AS2.

The annotated AS2 example creates and transforms points, checks half-open
rectangle bounds and inflates a region around its centre. Compile it with an
external AVM1 compiler; it is a tutorial source, not an independently executed
source-language acceptance test. Runtime bytecode construction is covered by
standard_rectangle.cpp. See doc/GEOMETRY_CLASSES.md and doc/RECTANGLE.md for scope.

<!-- example-catalog-api:start -->
## Назначение и особенности

AS2-исходник использует Point, Matrix и Rectangle: перенос координат, полуоткрытые границы и расширение прямоугольника. Рассчитан на внешний AVM1-компилятор; связан с байткодными тестами геометрии.


## Демонстрируемый script API

- `ColorTransform` — [GEOMETRY_CLASSES.md](../../../doc/GEOMETRY_CLASSES.md).
- `Matrix` — [GEOMETRY_CLASSES.md](../../../doc/GEOMETRY_CLASSES.md).
- `Point` — [GEOMETRY_CLASSES.md](../../../doc/GEOMETRY_CLASSES.md).
- `Rectangle` — [GEOMETRY_CLASSES.md](../../../doc/GEOMETRY_CLASSES.md).
- `pose.translate` — [GEOMETRY_CLASSES.md](../../../doc/GEOMETRY_CLASSES.md).
- `region.containsPoint` — [GEOMETRY_CLASSES.md](../../../doc/GEOMETRY_CLASSES.md).
- `region.inflate` — [GEOMETRY_CLASSES.md](../../../doc/GEOMETRY_CLASSES.md).
- `region.toString` — [GEOMETRY_CLASSES.md](../../../doc/GEOMETRY_CLASSES.md).
- `this.createEmptyMovieClip` — [GEOMETRY_CLASSES.md](../../../doc/GEOMETRY_CLASSES.md).
- `this.getNextHighestDepth` — [GEOMETRY_CLASSES.md](../../../doc/GEOMETRY_CLASSES.md).
- `tintCopy.toString` — [GEOMETRY_CLASSES.md](../../../doc/GEOMETRY_CLASSES.md).
- `trace` — [GEOMETRY_CLASSES.md](../../../doc/GEOMETRY_CLASSES.md).
- `transform.transformPoint` — [GEOMETRY_CLASSES.md](../../../doc/GEOMETRY_CLASSES.md).
- `transform.translate` — [GEOMETRY_CLASSES.md](../../../doc/GEOMETRY_CLASSES.md).

Это AS-эскиз/исходник; перечень имён не утверждает наличие отдельного C main
или полноту реализации каждого вызова.

[Описание в общем каталоге](../../README.md#example-embedding-geometry).
<!-- example-catalog-api:end -->
