# C89: малые кубические экстремумы

Исполняемый пример создаёт native cubic с Y-controls 0,12,-12,0, проверяет аналитические экстремумы ±2√3, масштабирует путь на 2^-600 и повторяет проверку после нормализации. Это геометрия public path API, а не добавление cubic-команд в managed MovieClip. Не нужны графика, шрифты, VM, GC, устройство или внешние SDK. Обычные ошибки возвращают 1; stdout успеха: `cubic extrema verified`. Пиксели не проверяются.

## Build / Сборка

From the standalone FlashVM component / Из отдельного компонента FlashVM:

```sh
cmake -S . -B build-cubic -DFVM_DEFS_FILE="$PWD/cmake/profiles/minimal.defs"
cmake --build build-cubic --target flashvm_examples_embedding_cubic_extrema_main
ctest --test-dir build-cubic -R '^flashvm_examples_embedding_cubic_extrema_main$' --output-on-failure
```

## API, владение и выполнение

README.ru.md документирует каждый использованный C API. Путь принадлежит вызывающему до destroy. Структуры command/matrix/bounds находятся на стеке; VM-roots и GC отсутствуют. Не допускается совместная мутация или удаление пути во время чтения. Все операции синхронны, без callbacks и устройств.

| API | Назначение, аргументы, ошибки и владение |
| --- | --- |
| [`fvm_path_create`](../../../include/flashvm/path.h#L50) | Создаёт native owner через out; проверять [FVM_OK](../../../include/flashvm/flashvm.h#L42) до использования, возможны [FVM_INVALID_ARGUMENT](../../../include/flashvm/flashvm.h#L45)/FVM_OUT_OF_MEMORY. Результат освобождается один раз через destroy. |
| [`fvm_path_add`](../../../include/flashvm/path.h#L54) | Заимствует path и копирует command; MOVE начинает контур, CUBIC хранит два controls и endpoint. Неверные команды/координаты дают [FVM_INVALID_ARGUMENT](../../../include/flashvm/flashvm.h#L45), выделение — [FVM_OUT_OF_MEMORY](../../../include/flashvm/flashvm.h#L44). Нет retained указателя на command. |
| [`fvm_path_bounds`](../../../include/flashvm/path.h#L71) | Заимствует неизменяемый path, пишет принадлежащие вызывающему четыре числа при [FVM_OK](../../../include/flashvm/flashvm.h#L42); неверные path/out дают [FVM_INVALID_ARGUMENT](../../../include/flashvm/flashvm.h#L45). Нет выделений, callbacks, GC или передачи владельца. |
| [`fvm_path_transform`](../../../include/flashvm/path.h#L65) | Синхронно изменяет native path по заимствованной matrix; неверная матрица/выход координат за ±1 000 000 дают [FVM_INVALID_ARGUMENT](../../../include/flashvm/flashvm.h#L45), копирование — [FVM_OUT_OF_MEMORY](../../../include/flashvm/flashvm.h#L44). Отказ сохраняет старый путь. |
| [`fvm_path_destroy`](../../../include/flashvm/path.h#L52) | Освобождает native owner и инвалидирует path; null допустим, статуса нет. После вызова запрещены чтение/правки и повторное освобождение. |

[Контракт и ограничения](../../../doc/CUBIC_EXTREMA_SCALING.ru.md). [main.c](main.c), [example_constants.h](example_constants.h). Требуется обычная двоичная арифметика без FTZ/fast-math; реальная платформа и полный SWF ≤ 8 не заявляются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../../../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../../../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../../../include/flashvm/flashvm.h#L44) |
| `fvm_path_add` | function | [include/flashvm/path.h:54](../../../include/flashvm/path.h#L54) |
| `fvm_path_bounds` | function | [include/flashvm/path.h:71](../../../include/flashvm/path.h#L71) |
| `fvm_path_create` | function | [include/flashvm/path.h:50](../../../include/flashvm/path.h#L50) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../../../include/flashvm/path.h#L52) |
| `fvm_path_transform` | function | [include/flashvm/path.h:65](../../../include/flashvm/path.h#L65) |
<!-- source-links:end -->
