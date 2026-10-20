# AS2: исходник → FIR → SWF

[English](README.md). Учебное подмножество: см. [покрытие](../../doc/COVERAGE.ru.md).

`main.as` — исходник; `expected.fir` — читаемое ожидаемое представление;
`expected.txt` — ожидаемые строки trace. Результат: **42**.

Все операнды здесь — числовые литералы. Поэтому адаптер имеет право выбирать
`f64`-арифметику. Строки, классы и динамический `+` исходного AS
этот учебный адаптер пока отклоняет. AS3-пример компилируется в AVM1 как
явно ограниченный числовой профиль; генератора ABC/AVM2 ещё нет.

Из корня репозитория, после [сборки](../../README.ru.md):

```sh
build/fir1/fir1_source as2 --input fir/v1/examples/as2/main.as build/fir1/as2.fir
build/fir1/fir1 check build/fir1/as2.fir
build/fir1/fir1 opt build/fir1/as2.fir build/fir1/as2.opt.fir
build/fir1/fir1 object build/fir1/as2.opt.fir build/fir1/as2.swo
build/linker/swl link build/fir1/as2.swf build/fir1/as2.swo
```

API примера: `trace(value)` выводит вычисленное число; `value` — единственный
аргумент. [Операции FIR](../../doc/FORMAT.ru.md), [CLI](../../compiler/README.ru.md),
[оптимизации](../../optimiser/README.ru.md). Запуск SWF и ожидаемые значения
описаны в [проверках](../../doc/TESTING.ru.md). Локальный просмотрщик:
`python fir/v1/view_docs.py`.

Пример `variables.as` показывает объявления с инициализацией и присваивания; каждое вычисление получает новое SSA-имя. Вывод: 36, затем 42.

## Расширенный числовой пример и оптимизатор

Исходник [syntax.as](syntax.as) подробно комментирует каждое вычисление; syntax.txt
содержит ожидаемую трассу. variables.as/variables.b показывает mutable bindings,
variables.fir — соответствующий SSA. expected.fir является читаемым входом FIR,
а не бинарным объектом. Файлы FIR можно править и повторно проверять.

```sh
build/native-fir/fir1_source as2 --input fir/v1/examples/as2/syntax.as build/native-fir/demo.fir
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
