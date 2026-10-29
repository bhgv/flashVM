# Целочисленные тождества в Dis и SWF

[English](README.md) · [Limbo с комментарием каждой строки](main.b) ·
[Точные правила и ограничения](../../doc/INTEGER_IDENTITIES.ru.md)

Сумматор складывает три копии индексов 0, 1, 2, 3 и затем 24:
3×6+24 = **42**. Умножение на ноль, остаток от деления на единицу и XOR с собой
дают ноль. Деление на единицу, нулевой сдвиг и AND с -1 сохраняют индекс.
Это правила целых чисел, не floating point. Динамический цикл проверяет peephole,
а не сводится к одной заранее известной константе.

Нужны наши C99-инструменты FIR с выбранным Coco/R SDK, линкер SWL и согласованный
DisVM. Исходник не компилируется через старый Limbo/bootstrap. Из корня репозитория:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/integer-identities/main.b build/identities.fir -g build/identities.json
build/fir/fir1 opt build/identities.fir build/identities.opt.fir
build/fir/fir1 dis build/identities.opt.fir build/identities.dis
build/runtime/disvm --jit --stats --inspect-words 0 2 build/identities.dis
build/runtime/disvm --no-jit --stats --inspect-words 0 2 build/identities.dis
build/fir/fir1 object build/identities.opt.fir build/identities.swo
build/linker/swl link build/identities.swf build/identities.swo
```

Dis сообщает DATA 0 42 и DATA 8 1: результат и признак исполнения. Настоящий
FlashVM host [run_flashvm.c](../../tests/run_flashvm.c) выводит 42 для SWF;
[инструкция сборки](../../doc/TESTING.ru.md) объясняет статические зависимости.
Raw FIR тоже исполняется в Dis, но raw SWO отвергает оставшиеся checked-операции
с сохранением прежнего выходного файла.

Для встраивания `fir_source_debug` создаёт FIR и необязательный source sidecar;
`fir_run` с optimise=1 выдаёт FIR_OBJECT или FIR_DIS через тот же проход.
[Исполняемая C99-проверка](../../tests/integer_identities_api.c) показывает
освобождение потоков, заимствование снимков и восстановление после отказа raw.
Потоками владеет вызывающий код; module/snapshot нельзя удерживать после вызова.
Python не становится зависимостью runtime; кучи и ABI двух VM не меняются.

## Исполняемый библиотечный клиент

[main.c](main.c) объясняет каждую строку кода. Обычная сборка FIR создаёт
`build/fir/fir1_integer_identities_example`; установленный SDK содержит программу,
исходник и согласованные библиотеки. Запуск:

```sh
build/fir/fir1_integer_identities_example fir/v1/examples/integer-identities/main.b build/identities-api.dis dis
build/fir/fir1_integer_identities_example fir/v1/examples/integer-identities/main.b build/identities-api.swo object
```

Клиент готовит полный результат до открытия выходного файла. Ошибка последующей
записи может оставить префикс; атомарное переименование не обещается. Проверки
перенесённого SDK заново компилируют клиент только с установленными файлами,
побайтно сравнивают оба результата с CLI и исполняют Dis в интерпретаторе/JIT.
Выданный SWO всё ещё требует отдельного шага линковки SWL.

Для совпадения байтов CLI клиент явно материализует канонический optimized FIR
между FIR_OPT и backend. Прямая оптимизация в памяти сохраняет прежние SSA и
может резервировать другие слоты; см. [контракт](../../doc/INTEGER_IDENTITIES.ru.md).
