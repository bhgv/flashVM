# MovieClip shape hitTest / Проверка попадания в фигуру

hitTest(x, y, true) проверяет заливки и обводки собственных векторных путей и
дочерних клипов в глобальных координатах. Вариант с false и hitTest(otherClip)
сохраняют проверку bounding box. Проверка доступна без графического backend.
Учитываются правило nonzero/even-odd, отверстия, неявное замыкание заливок,
квадратичные/кубические кривые, круглые окончания/соединения обводок и текущие
матрицы отображения. Флаг shapeFlag использует truthiness AS, включая строки.

Новый C89 API [fvm_path_hit_test](../include/flashvm/path.h#L75) работает с той же редактируемой векторной
моделью. Для кривых используются адаптивные хорды с допуском 0.0001 единицы
координат и глубиной не более 20. Граница заливки проверяется с допуском
0.000001. Это численное приближение, не обещание точного поведения Flash
на границах/субпикселях. Обводки масштабируются тем же приближением максимального
масштаба столбцов матрицы, что текущая отрисовка; полная анизотропная геометрия,
bitmap/text/filter hit testing и clipDepth/masks ещё открыты.

Тесты path_hit и movieclip_shape_hit проверяют отверстия/правила заполнения,
треугольник против bounding box, кривые, обводки, неявное замыкание, отражение,
родительские преобразования, флаг-строку, большие координаты, malformed API inputs,
queue/file transfer и GC. Файл проверяется только в профилях с файловыми каналами.
Ресурсные пути SWF используют тот же hitTest, но полный эталонный корпус SWF
и завершение этапа 3 ещё не подтверждены.

The point/shape overload now traverses vector fills, round strokes and child
clips in world coordinates without requiring AGG or Cairo. Bounding-box overloads
remain available. The shared C89 path query supports fill rules, holes, implicit
fill closure and adaptive Bezier subdivision. Numerical boundary tolerance,
subdivision depth and current stroke-scale approximation are explicit limits.
Bitmap/text/filter and masked hit testing remain open. The named suites cover
geometry, reflected/nested transforms, truthiness, transport and GC; this does
not certify full MovieClip or historical Flash conformance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_path_hit_test` | function | [include/flashvm/path.h:75](../include/flashvm/path.h#L75) |
<!-- source-links:end -->
