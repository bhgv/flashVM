# LIMBO: исходник → FIR → SWF

[English](README.md). Учебное подмножество: см. [покрытие](../../doc/COVERAGE.ru.md).

`main.b` — исходник; `expected.fir` — читаемое ожидаемое представление;
`expected.txt` — ожидаемые строки trace. Результат: **-2147483648**.

Limbo экспортирует `value(): int`. Адаптер встраивает вычисление результата в
`@main` и добавляет trace. Это тестовая обвязка, не реализация Inferno module ABI.

Из корня репозитория, после [сборки](../../README.ru.md):

```sh
build/fir1/fir1_source limbo --input fir/v1/examples/limbo/main.b build/fir1/limbo.fir
build/fir1/fir1 check build/fir1/limbo.fir
build/fir1/fir1 opt build/fir1/limbo.fir build/fir1/limbo.opt.fir
build/fir1/fir1 swf build/fir1/limbo.opt.fir build/fir1/limbo.swf
```

API примера: `trace(value)` выводит вычисленное число; `value` — единственный
аргумент. [Операции FIR](../../doc/FORMAT.ru.md), [CLI](../../compiler/README.ru.md),
[оптимизации](../../optimiser/README.ru.md). Запуск SWF и ожидаемые значения
описаны в [проверках](../../doc/TESTING.ru.md). Локальный просмотрщик:
`python fir/v1/view_docs.py`.

Пример `variables.b` показывает объявления с инициализацией и присваивания; каждое вычисление получает новое SSA-имя. Учебный вход печатает 36.
