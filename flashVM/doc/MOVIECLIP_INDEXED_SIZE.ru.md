# Индексные размеры MovieClip — v0.379

ActionGetProperty и общий C-сервис теперь читают ширину (индекс 8) и высоту
(индекс 9) текущей векторной геометрии. Для пустого клипа возвращается ноль.
Контуры преобразуются локальной матрицей клипа и матрицами его потомков;
учитываются поворот, отражение, shear и смещения потомков. Матрицы предков
не нужны: даже вырожденный масштаб родителя не препятствует чтению.
Используется существующее расширение границ на толщину линии.

Подмена именованных `_width`/`_height` не перехватывает индексное чтение.
Преобразование индекса по-прежнему предшествует выбору цели и может выполнять
скрипт с настоящим GC. Входная цель остаётся корнем при отсоединении клипа.
Результат можно записать поверх входного значения; при ошибке или OOM выход
сохраняется. В v0.385 добавлена индексная запись
в ограниченной vector модели (MOVIECLIP_DIMENSION_WRITES.ru.md). Публичный ABI,
идентификаторы callbacks и формат графа не изменяются.

[GC:] Обход геометрии держит нативный барьер сборки и копирует пути/потомков
каждого узла до вызова заменяемых getter преобразований. Удаление соседа не
инвалидирует итератор и не освобождает объект из снимка. Захваченные потомки
участвуют в текущем чтении; следующее чтение видит удаление. Состояние более
поздних узлов захватывается при достижении узла. Это ограниченная модель
существующего движка, а не полное подтверждение Flash intrinsic callbacks.

Открыты именованные размеры и setters, точное twip-округление Flash, границы
bitmap/text/filter/mask/scrollRect, точные стыки и окончания линий, полный
ресурсный display list. Сохраняются double-геометрия и ограничение глубины;
очень большие координаты могут терять точность размеров. Нового прогона
оригинального Flash Player и проверки другой ОС нет.

Сопоставление: [width/height и local_bounds закреплённого Ruffle](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/display_object.rs).
Это основание выбора пространства родителя, не сертификат всей геометрии.

Проверки: movieclip_indexed_size — host/SWF4–8, пустые и преобразованные
клипы, потомки, shear, вырожденные предки, alias и проверка записи, обновлённая в v0.385;
movieclip_indexed_size_callbacks — удаление из getter, обход именованных
размеров, ошибки, настоящий GC при преобразовании индекса, отсоединение,
исключения и восстановление корней/барьера; movieclip_indexed_size_allocation —
все границы выделения для обоих чтений, сохранение выхода, GC и повтор;
jit_indexed_size — интерпретатор, явный/автоматический JIT и отсутствующая цель.
Все девять стадий остаются открытыми.

Validation v0.379: all 785 AGG / 785 Cairo / 766 minimal Linux Release checks
have passing evidence after nine executable-permission repairs on eight ELF
files and targeted reruns (four repair operations AGG, three Cairo, two minimal).
Eight restricted heap/global ASan+UBSan checks pass: the four new dimension
checks plus standard MovieClip, AVM1/JIT display properties and geometry
allocation. Stack ASan and LeakSanitizer remain disabled. No new external SWF
oracle or foreign-OS runtime acceptance is claimed. All nine stages remain open.
