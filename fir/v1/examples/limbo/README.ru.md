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
build/fir1/fir1 object build/fir1/limbo.opt.fir build/fir1/limbo.swo
build/linker/swl link build/fir1/limbo.swf build/fir1/limbo.swo
```

API примера: `trace(value)` выводит вычисленное число; `value` — единственный
аргумент. [Операции FIR](../../doc/FORMAT.ru.md), [CLI](../../compiler/README.ru.md),
[оптимизации](../../optimiser/README.ru.md). Запуск SWF и ожидаемые значения
описаны в [проверках](../../doc/TESTING.ru.md). Локальный просмотрщик:
`python fir/v1/view_docs.py`.

Пример `variables.b` показывает объявления с инициализацией и присваивания; каждое вычисление получает новое SSA-имя. Учебный вход печатает 36.

## Расширенный числовой пример и оптимизатор

Исходник [syntax.b](syntax.b) подробно комментирует каждое вычисление; syntax.txt
содержит ожидаемую трассу. variables.as/variables.b показывает mutable bindings,
variables.fir — соответствующий SSA. expected.fir является читаемым входом FIR,
а не бинарным объектом. Файлы FIR можно править и повторно проверять.

```sh
build/native-fir/fir1_source limbo --input fir/v1/examples/limbo/syntax.b build/native-fir/demo.fir
build/native-fir/fir1 check build/native-fir/demo.fir
build/native-fir/fir1_opt build/native-fir/demo.fir build/native-fir/demo.opt.fir
build/native-fir/fir1 object build/native-fir/demo.opt.fir build/native-fir/demo.swo
build/native-linker/swl link build/native-fir/demo.swf build/native-fir/demo.swo
```

Потоки/буферы C API принадлежат вызывающему; полученный FIR не содержит указателей
на структуры VM. Парсеры сериализуются, callbacks снимков заимствуют IR лишь до
возврата. Оптимизация сохраняет эффекты trace/каналов и профили; f64 не получает
fast-math преобразований. AS3 здесь не выдаёт ABC; Limbo — только i32 value(),
без общего ABI модулей/каналов. Cross-компилятор запускайте на его целевой машине.
[Сборка native/cross](../../../../BUILDING.ru.md),
[C-пример всех стадий](../../../../linker/examples/ide/README.ru.md).
