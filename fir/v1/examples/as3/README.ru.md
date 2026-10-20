# AS3: исходник → FIR → SWF

[English](README.md). Учебное подмножество: см. [покрытие](../../doc/COVERAGE.ru.md).

`main.as` — исходник; `expected.fir` — читаемое ожидаемое представление;
`expected.txt` — ожидаемые строки trace. Результат: **5.75**.

Все операнды здесь — числовые литералы. Поэтому адаптер имеет право выбирать
`f64`-арифметику. Строки, классы и динамический `+` исходного AS
этот учебный адаптер пока отклоняет. AS3-пример компилируется в AVM1 как
явно ограниченный числовой профиль; генератора ABC/AVM2 ещё нет.

Из корня репозитория, после [сборки](../../README.ru.md):

```sh
build/fir1/fir1_source as3 --input fir/v1/examples/as3/main.as build/fir1/as3.fir
build/fir1/fir1 check build/fir1/as3.fir
build/fir1/fir1 opt build/fir1/as3.fir build/fir1/as3.opt.fir
build/fir1/fir1 object build/fir1/as3.opt.fir build/fir1/as3.swo
build/linker/swl link build/fir1/as3.swf build/fir1/as3.swo
```

API примера: `trace(value)` выводит вычисленное число; `value` — единственный
аргумент. [Операции FIR](../../doc/FORMAT.ru.md), [CLI](../../compiler/README.ru.md),
[оптимизации](../../optimiser/README.ru.md). Запуск SWF и ожидаемые значения
описаны в [проверках](../../doc/TESTING.ru.md). Локальный просмотрщик:
`python fir/v1/view_docs.py`.

Пример `variables.as` показывает объявления с инициализацией и присваивания; каждое вычисление получает новое SSA-имя. Вывод: 36, затем 42.
