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
