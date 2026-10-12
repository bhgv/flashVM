# flash.geom.Rectangle

Rectangle добавлен в управляемое пространство flash.geom и реестр C API
`flash.geom.Rectangle`. Данные x/y/width/height остаются обычными свойствами.
Доступны left/top/right/bottom, topLeft/bottomRight/size; последние три
возвращают новые Point. Изменение левой/верхней стороны сохраняет противоположную
сторону; установка правой/нижней меняет размер. Значения сторон складываются
по правилам AS2 +, включая строки и NUL.

Реализованы clone, setEmpty, isEmpty, inflate, inflatePoint, offset, offsetPoint,
contains, containsPoint, containsRectangle, intersection, intersects, union,
equals, toString. Проверенная основа — числовая геометрия. contains включает
левую/верхнюю границы и исключает правую/нижнюю. Касающиеся прямоугольники дают
пустое пересечение. setEmpty присваивает поля в порядке height/width/y/x.
Частичный конструктор сохраняет undefined; без аргументов вызывается публичный
setEmpty. Объединение/пересечение и intersects учитывают публичные методы
isEmpty/clone/intersection. Фабрики удерживают Point и Rectangle через корни GC.

standard_rectangle.cpp проверяет все публичные методы и производные свойства,
границы, пустые/отрицательные размеры, ActionNewMethod через flash.geom,
вызов переопределённого intersection, очередь/файл/loopback TCP и GC в принимающем контексте
без стандартной библиотеки. rectangle_allocation.cpp перебирает отказы памяти
для clone, intersection, union и Point-свойств; исходные области сохраняются.
Установка библиотеки с отказами покрыта geometry_allocation.cpp.
Учебный AS2 пример: examples/embedding/geometry/example_as2.as.

Это контрольная точка, а не полная совместимость. Точный порядок всех
getter/valueOf, строковые сравнения, все случаи NaN/Infinity, абстрактное
сравнение equals и Math overrides требуют эталонных сценариев и доработки.
Loopback TCP проверяется в обоих направлениях в одном процессе; отдельные
процессы и удалённые адреса для Rectangle пока не проверены. Transform, ColorTransform
и остальные задачи графики/текста/медиа остаются открытыми.

## English

Rectangle is a managed flash.geom class with ordinary coordinate/extent fields,
seven derived accessors and the listed fifteen instance methods. Basic numeric
geometry uses half-open point bounds; touching rectangles have empty intersection.
Default construction dispatches public setEmpty, while partial construction keeps
undefined fields. Point-valued accessors and Rectangle factories retain their
class/prototype graphs across queue/file serialization and collection.

standard_rectangle.cpp exercises every method/accessor, AVM1 NewMethod,
selected override dispatch, queue/file/loopback TCP restoration without installed standard
classes and GC. rectangle_allocation.cpp injects allocation failure through
result factories and verifies unchanged inputs; standard installation injection
is covered separately. The AS2 tutorial is intended for an external compiler.

Complete getter/coercion order, string comparisons, exceptional numeric values,
abstract equals and overridden Math behavior remain conformance work. Bidirectional loopback TCP uses separate contexts in one process;
independent-process and remote-host Rectangle coverage is pending. No roadmap stage or full Flash equivalence is certified.

## Проверки v0.34 / Validation

Linux: headless 190, AGG 200, Cairo 200, portable 181. Один portable тест
не запустился из-за execute permissions и прошёл после восстановления прав.
Strict C89/C++98: 187 тестов/примеров с ASan/UBSan; ASan stack и LeakSanitizer
отключены из-за среды, heap/global ASan и UBSan включены;
container_exceptions проверен только UBSan. Остальные ОС не проверены.

All listed Linux cases passed, including the permission-related portable retry.
The sanitizer limitations are explicit above; this is not cross-platform or
complete Flash Player acceptance. Generated validation logs remain outside Git.
