# BitmapData.hitTest

## Русский

v0.57 добавляет hitTest(firstPoint, firstAlphaThreshold, secondObject,
secondBitmapPoint, secondAlphaThreshold). firstPoint и secondBitmapPoint
задают положение верхнего левого пикселя соответствующего bitmap в общей
системе координат. Матрицы сцены не применяются. secondObject может быть
Point, Rectangle, BitmapData или обычным объектом с соответствующими полями.

Геометрия приводится к signed 32-bit через усечение и modulo 2^32; разности
и пересечение вычисляются в double, чтобы крайние координаты не вызывали
signed overflow на ILP32. Прямоугольники с неположительными размерами пусты;
правая/нижняя границы исключены. Alpha сравнивается через >= threshold.
Порог ограничивается 0..255 после signed conversion, пропущенный второй
порог равен 1. При проверке Point нулевой первый порог считается 1; при
Rectangle/BitmapData ноль допускает прозрачный пиксель. Opaque bitmap имеет
alpha=255 и участвует как непрозрачный прямоугольник.

Все публичные getters/conversions завершаются до чтения storage. Затем
повторно проверяется disposal обоих bitmap. Ошибочные вызовы возвращают
число: -1 invalid/disposed receiver, -2 invalid first point, -3 invalid
comparison object, -4 missing/invalid second bitmap point. Ошибки VM из
getters/conversions сохраняются, включая OOM. Getter side effects не
откатываются. Native callback 1546 восстанавливается из графа каналов.

bitmap_hit_test.cpp проверяет пороги/границы/пересечения, self comparison,
opaque pixels, modulo координаты, duck typing, error codes, getter disposal,
queue/file/loopback TCP и GC без standard_install в принимающем контексте.
bitmap_hit_test_allocation.cpp проверяет каждый new/new[] boundary всех трёх
ветвей, неизменность пикселей и allocation balance после уничтожения VM.
Профили: headless, AGG, Cairo, portable, heap/global ASan+UBSan. Stack ASan
и leak scanner в этом окружении отключены.

Открыто: полная сверка Flash accessor/prototype/version coercion, отсутствующий
либо explicit undefined second point, удалённые процессы и complete stage 4.
Тесты реализации не заменяют эталонную проверку исторического Flash.

## English

v0.57 adds point, rectangle and bitmap alpha hit tests in a shared coordinate
space, without scene transforms. Public geometry is converted to signed
32-bit coordinates using truncation and modular conversion; double differences
avoid signed overflow. Nonpositive rectangles are empty and edges are half-open.
Alpha passes >= threshold. Thresholds clamp to 0..255; the omitted second
threshold is 1. Point tests treat a zero first threshold as 1, whereas rectangle
and bitmap tests accept transparent pixels at zero. Opaque storage has alpha 255.

Public getters and conversions finish before native pixel reads. Storage is
revalidated afterwards. Invalid receiver/first point/comparison/second bitmap
point return -1/-2/-3/-4 respectively. Getter errors, including OOM, propagate;
getter side effects remain. Stable callback 1546 survives graph transport.

Tests exercise overloads, thresholds, boundaries, integer extremes, aliases,
duck typing, numeric errors, getter disposal, queue/file/loopback TCP/GC and allocation
failure/balance. Headless, AGG, Cairo, portable and heap/global ASan+UBSan are
checked, with stack ASan and leak scanning disabled. Full historical Flash
accessor/prototype/version conformance and roadmap stage 4 remain open.
