# Синтаксические примеры Limbo

[English](README.md). Эти файлы проверяют широкий синтаксический фронтенд,
а не отдельный числовой backend FIR. Здесь не создаются SWO, ABC, Dis или SWF.

| File | Purpose |
| --- | --- |
| [Syntax.b](Syntax.b) | Nonempty lists, array initializer/wildcard, slice, loop/alt labels |

Собирайте из каталога языка с внешним Coco/R, исполняемым на ХОСТЕ:

```sh
cmake -S . -B build -DCOCOR_ROOT=/host/coco-r-C \
  -DCOCOR_EXECUTABLE=/host/coco-r-C/sources/cocor
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
build/frontend/limbo_parse examples/Syntax.b
```

Код 0 означает принятый синтаксис, 1 — ошибку синтаксиса/кодировки, 2 — ошибку
аргументов/ввода-вывода. Парсер не исполняет trace, каналы и методы, не разрешает
типы/классы. AS требует явных точек с запятой. Limbo использует # комментарии и
пробел в wildcard * =>. C API принимает принадлежащие вызывающему UTF-8 байты/
потоки; владение исходником не передаётся. После ошибки можно вызвать снова;
вызовы одного нерекурсивно-безопасного парсера сериализуйте снаружи.

[Покрытие/API языка](../README.ru.md), [фронтенд](../frontend/README.ru.md),
[рабочий числовой FIR](../../../fir/v1/doc/SOURCE_SYNTAX.ru.md),
[нативная/cross сборка](../../../BUILDING.ru.md),
[все примеры компиляторов](../../examples.ru.md). Драйвер cross-сборки исполняется
на выбранной ОС/CPU; команда выше является нативным запуском.
