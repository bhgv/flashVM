# flash.geom.Point и Matrix / Point and Matrix

Реализована управляемая модель `flash.geom.Point` и `flash.geom.Matrix` для
AS2: классы доступны через `flash.geom`, а C API использует полные имена
`flash.geom.Point` и `flash.geom.Matrix` в `fvm_construct`.

Point предоставляет x/y, вычисляемую length, add, subtract, clone, equals,
normalize, offset, toString и статические distance, interpolate, polar.
Matrix предоставляет a/b/c/d/tx/ty, clone, concat, createBox,
createGradientBox, deltaTransformPoint, identity, invert, rotate, scale,
toString, transformPoint и translate. Конкатенация применяет другую матрицу
после текущей. createGradientBox переводит стандартные координаты градиента
в прямоугольник и подходит для MovieClip.beginGradientFill.

Конструкторы сохраняют переданные значения полей; вызов без аргументов
создаёт нулевую точку или единичную матрицу. При частичном наборе аргументов
пропущенные поля остаются undefined. Основные вычисления используют ToNumber.
Методы создания новых экземпляров удерживают классы через корни GC: после
передачи объекта получатель не обязан устанавливать стандартную библиотеку.
Объекты, прототипы, классы и общие ссылки передаются обычным сериализатором;
новые идентификаторы нативных методов не зависят от адресов процесса.

Проверки: `standard_geometry.cpp` — namespace, повторная установка, длина,
частичный конструктор, клонирование, equals, преобразования, обратная матрица,
createGradientBox, восстановление Point/Matrix через очередь и Matrix через
файл, GC восстановленного графа. `geometry_allocation.cpp` перебирает отказы
выделения при clone и установке стандартной библиотеки, проверяет отсутствие
частично опубликованного namespace и сохранность исходной точки.

Ограничения: это не завершение этапов 1–4. Пока нет полной проверки поведения
геометрических методов в эталонном Flash Player. В v0.33 добавлена строковая арифметика add/offset/translate, interpolate и
createGradientBox. Порядок getter/setter проверен для add и offset; normalize
читает публичную length, invert вызывает публичный deltaTransformPoint.
Полный порядок всех getter/valueOf, абстрактное сравнение equals и
переопределения Math.sin/cos/sqrt ещё требуют проверки/реализации. Сетевой транспорт этих новых типов требует отдельного теста.
Остальные flash.geom классы и полная интеграция Transform остаются открытыми.

## English

Managed AS2 Point and Matrix classes are installed under flash.geom. The C
constructor registry uses their fully qualified names. All listed public methods
are present; basic numeric behavior is exercised by standard_geometry.cpp.
Default constructors create the zero point and identity matrix; partial
constructors retain undefined fields and explicit values are copied without
initial coercion. concat applies the supplied matrix after the current one.
createGradientBox maps normalized gradient coordinates to a rectangle.

Factory callbacks retain their constructor graph through managed GC roots.
Queue/file graph transport restores stable native callback IDs and captured
classes, allowing method calls without installing standard classes in the
receiving context. Allocation injection covers clone and atomic standard-library
namespace publication. Tests cover restored graph collection and method calls.

This is a compatibility checkpoint, not completion of any roadmap stage.
v0.33 adds AS2 string addition to add/offset/translate, interpolate and
createGradientBox. normalize reads dynamic length and skewed invert dispatches
through public deltaTransformPoint. Remaining conformance work includes full
getter/coercion order, equals abstract comparison and overridden Math functions.
Network transport needs dedicated coverage. Other flash.geom classes and full
Transform integration remain open. No complete Flash Player equivalence is claimed.

## Проверенная контрольная точка v0.32 / Validation checkpoint

Linux CMake/CTest: headless 187, AGG 197, Cairo 197, portable 178; все наборы
прошли. После расширения числовых сценариев два затронутых теста повторно
прошли во всех четырёх конфигурациях. Strict C89/C++98: 184 теста/примера
с ASan/UBSan; ASan stack и LeakSanitizer отключены из-за ограничений среды,
heap/global ASan и UBSan включены; container_exceptions проверен только UBSan.
Первый headless запуск не смог запустить два файла из-за execute permissions;
после восстановления прав полный повторный набор прошёл. Это не проверка
Windows/macOS/Android или полной совместимости Flash Player.

All four Linux profiles passed. The extended numeric tests were rebuilt and
rerun in each profile. The strict language/sanitizer run passed 184 programs,
with the explicitly stated instrumentation limitations above. No other platform
or complete Flash Player conformance is certified. Generated logs stay outside Git.


## Диспетчеризация v0.33 / Dispatch checkpoint

geometry_dispatch.cpp проверяет конкатенацию строк и сохранение NUL, порядок
чтения y/x при add и записи x/y при offset, динамическую length при normalize,
переопределение deltaTransformPoint при invert и строковые координаты
interpolate/createGradientBox. geometry_allocation.cpp дополнен перебором
отказов памяти для add со строковым результатом и сохранности исходной точки.

The dispatch regression covers string/NUL addition, public getter/setter order,
dynamic normalize length, overridden inverse translation and string interpolation/
gradient-box translations. Allocation injection additionally covers string-valued
Point.add while retaining the original coordinates on failure.

Проверки v0.33: headless — 188 (четыре BAD_COMMAND из-за прав исполнения
успешно повторены), AGG/Cairo — по 198, portable — 179; все выполненные тесты
прошли. Strict C89/C++98 с ASan/UBSan — 185 тестов/примеров; ограничения
инструментирования совпадают с описанными для v0.32. Полная сверка Flash
и реальные сборки других ОС остаются открытыми.

v0.33 validation passed 188 headless cases (four permission-related launch
failures passed on retry), 198 per graphical profile, 179 portable and 185
strict-language/sanitizer programs. The v0.32 instrumentation limitations apply.
