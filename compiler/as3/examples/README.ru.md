# Синтаксические примеры AS3

[English](README.md). Эти файлы проверяют широкий синтаксический фронтенд,
а не отдельный числовой backend FIR. Здесь не создаются SWO, ABC, Dis или SWF.

| File | Purpose |
| --- | --- |
| [in.as](in.as) | Ordinary membership, for-in separator and parenthesized NoIn initializer |
| [Counter.as](Counter.as) | Typed instance member, constructor and method body |
| [Functions.as](Functions.as) | Defaults/rest, for-each, namespace qualifiers, get/set, catches/finally |
| [ICounter.as](ICounter.as) | Declaration-only interface signature |

Собирайте из каталога языка с внешним Coco/R, исполняемым на ХОСТЕ:

```sh
cmake -S . -B build -DCOCOR_ROOT=/host/coco-r-C \
  -DCOCOR_EXECUTABLE=/host/coco-r-C/sources/cocor
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
build/frontend/as3_parse examples/in.as
```

Код 0 означает принятый синтаксис, 1 — ошибку синтаксиса/кодировки, 2 — ошибку
аргументов/ввода-вывода. Парсер не исполняет trace, каналы и методы, не разрешает
типы/классы. AS-примеры также показывают автоматическую вставку точек с запятой. Limbo использует # комментарии и
пробел в wildcard * =>. C API принимает принадлежащие вызывающему UTF-8 байты/
потоки; владение исходником не передаётся. После ошибки можно вызвать снова;
вызовы одного нерекурсивно-безопасного парсера сериализуйте снаружи.

[Покрытие/API языка](../README.ru.md), [фронтенд](../frontend/README.ru.md),
[рабочий числовой FIR](../../../fir/v1/doc/SOURCE_SYNTAX.ru.md),
[нативная/cross сборка](../../../BUILDING.ru.md),
[все примеры компиляторов](../../examples.ru.md). Драйвер cross-сборки исполняется
на выбранной ОС/CPU; команда выше является нативным запуском.

## Новый пример парсера

[XmlVector.as](XmlVector.as) — только синтаксис, без выполнения и генерации объектов.
Цель — проверить читаемый исходник через тот же C API, который вызывает драйвер.
Хост владеет входными байтами/FILE и структурой диагностики; парсер ничего не
закрывает и не передаёт владение. Include, типы, символы и события не выполняются.
Каждая непустая строка примера снабжена пояснением. Ожидается код возврата 0
и отсутствие генерируемого файла. Ошибка синтаксиса даёт 1, ошибка запуска/I/O — 2.

```sh
build/frontend/as3_parse examples/XmlVector.as
```

[Контракт грамматики и API](../README.ru.md) · [Сборка](../../../BUILDING.ru.md)

[XmlReferences.as](XmlReferences.as) — новый синтаксический пример версии 0.5.0. Проверка: `build/frontend/as3_parse examples/XmlReferences.as`. Парсер возвращает 0 при корректном синтаксисе; исполнения и проверки типов нет.

[UnicodeNames.as](UnicodeNames.as) — имена и escape-последовательности Unicode 3.2; синтаксический пример. Проверка: `build/frontend/as3_parse examples/UnicodeNames.as`.

[RegexpVectors.as](RegexpVectors.as) — regexp, Vector, CONFIG; только синтаксис. Проверка: `build/frontend/as3_parse examples/RegexpVectors.as`.
