# Параметры функций и блоков FIR

[main.fir](main.fir) — читаемый структурный пример цикла суммы с параметрами
блока вместо phi. Он показывает сигнатуру, явные аргументы рёбер, file/loc и
export. Парсер не вычисляет сумму, не строит SSA-граф и не генерирует SWO.
Текущий исполняемый backend ещё отклоняет этот профиль.

```sh
cmake -S . -B build -DCOCOR_ROOT=/host/coco-r-C \
  -DCOCOR_EXECUTABLE=/host/coco-r-C/sources/cocor
cmake --build build
build/fir1 syntax examples/structural/main.fir
build/frontend/firsyn_parse examples/structural/main.fir
ctest --test-dir build --output-on-failure
```

Команды выполняются из fir/v1; ожидается статус 0, без вывода результата или
файла. Нужны C99, CMake 3.16+ и внешний host Coco/R с matching frames. Python 3
нужен тестам. В C используйте firsyn_check для принадлежащего хосту буфера или
firsyn_read для FILE; оба синхронны, возвращают bool-статус и не закрывают вход.
Ресурсы/диагностику освобождает хост. Сериализуйте обращения к одному парсеру.
Порядок переменных, совпадение типов переходов, доминирование и opcode-эффекты
требуют отдельного верификатора. Не передавайте успешный syntax-результат напрямую
оптимизатору как проверенный FirModule. Для object нужна успешная проверка
исполняемого профиля; отрицательный тест подтверждает сохранность выходного файла.

[Контракт API/грамматики](../../frontend/README.ru.md) ·
[Каталог](../README.ru.md) · [Сборка](../../../../BUILDING.ru.md).
