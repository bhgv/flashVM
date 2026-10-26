# Наш Limbo → FIR → Dis: подробный пример

[English](README.md) · [Цель/границы/API](../../doc/DIS_BACKEND.ru.md) ·
[Комментированный исходник](main.b) · [Эквивалент FIR](main.fir) · [C99-клиент библиотек](main.c).

Пример проходит через наш Coco/R-парсер, lowering, verifier и C99 Dis-emitter.
`main.b` начинает с INT32_MAX, переполняет сложение и умножение, затем вычитает
единицу: получается **2147483647**. Каждая промежуточная операция имеет i32
wrap-семантику, хотя слово NC64 занимает восемь байт. `main.fir` явно описывает
те же операции и также пригоден для существующей SWO-цели.

Из корня репозитория, после отдельной сборки FIR и нашего DisVM:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/dis/main.b build/dis-example.fir
build/fir/fir1 dis build/dis-example.fir build/dis-example.dis
build/editor/disvm --jit --stats --inspect-words 0 1 build/dis-example.dis
build/editor/disvm --no-jit --stats --inspect-words 0 1 build/dis-example.dis
```

Пути `build/fir` и `build/editor` приведены как пример; подставьте каталоги
своих инструментов. `DATA 0 2147483647` — значение первого реально выполненного
trace, не заготовленная надпись об успехе. Компилятор не запускает программу,
чтобы вычислить ответ. Временный компилятор/ассемблер Inferno здесь не участвует.

Для встраивания без дочерних процессов используйте собранный C99-клиент:

```sh
build/fir/fir1_dis_example fir/v1/examples/dis/main.b build/dis-example.dis
build/editor/disvm --jit --inspect-words 0 1 build/dis-example.dis
```

`main.c` вызывает `fir_source` и `fir_run(FIR_DIS)`, владеет промежуточными
потоками, сохраняет прежнее назначение до успешной компиляции, проверяет I/O и
закрывает все открытые потоки. Библиотеки не печатают и не завершают процесс.
В IDE эту блокирующую работу нужно выполнять в build worker, передавая
копируемые результаты владельцу UI через сервисный канал.

Линейный пример использует i32-константы/copy/wrap-арифметику, захват trace и
return. Цель также поддерживает bool и проверенный CFG, как показано ниже. Нет импортов, экспортированного Limbo ABI `value`, строк, исходных
каналов и Dis SBL. Возвращаемое значение учебного исходника становится trace
FIR `@main`; без `--inspect-words` оно не печатается. Полная графическая IDE
пока не компилируется этим подмножеством.

В установленном FIR-пакете можно заново собрать C99-клиент только из его SDK:

```sh
cc -std=c99 -I include examples/dis/main.c lib/libfir1_ir.a lib/libfir1_source_api.a lib/libfir1_io.a -o dis-client
./dis-client examples/dis/main.b answer.dis
```

## Терминальные решения и флаги исполнения

[decision.b](decision.b) использует наш Coco/R-профиль; [decision.fir](decision.fir)
явно показывает CFG. Переполнение даёт отрицательное wrapped; внешняя ветка >=
не посещается, вложенное равенство выбирает 42. Все ветви возвращают значение:
слияния SSA и изменения bindings внутри ветвей здесь нет.

```sh
build/fir/fir1_source limbo --input fir/v1/examples/dis/decision.b build/decision.fir
build/fir/fir1 dis build/decision.fir build/decision.dis
build/editor/disvm --jit --max-instructions 1000 --inspect-words 0 6 build/decision.dis
```

Три статические точки trace дают значения `[0, 42, 0]`, затем флаги `[0, 1, 0]`.
Выбранное значение находится по байту 8, его флаг — по байту 32. Настоящий
возврат нуля имел бы флаг 1; непосещённая ячейка содержит ноль с флагом 0.
Порядок слотов физический, повторное посещение сохраняет последнее значение:
это не хронологический журнал. После оптимизации число точек trace нужно
пересчитать, потому что недостижимые ветви вместе со слотами могут исчезнуть.

Тот же исходник работает через `fir1 object` и SWL в SWF8. Там учебная обвязка
печатает выбранное значение, а Dis сохраняет его для инспекции. Шесть сравнений
принимаются только в терминальных if/else с явными скобками. Обычные операторы
внутри ветвей, продолжение после решения, исходные while/for, phi, импорты и
Dis SBL остаются следующими этапами.

Проверка установленной поставки:

```sh
python3 tools/test_installed_workspace.py --prefix out --scratch build/package-tmp --fir-dis-only
```

Она заново связывает C99-клиент из заголовков/библиотек пакета, сравнивает Dis
с CLI побайтно и исполняет линейный/условный примеры в обоих режимах из
перенесённого каталога с пробелами.

## Binary64 из AS

[reals.as](reals.as) — полностью комментированный числовой пример AS3. Его
Coco/R frontend создаёт FIR, затем наш C99-emitter пишет Dis без Inferno-emitter.
Библиотечный [main.c](main.c) принимает третий аргумент `as3`; без него сохранён
Limbo. Это тот же публичный API, не отдельная реализация backend.

```sh
build/fir/fir1_source as3 --input fir/v1/examples/dis/reals.as build/reals.fir --module dis_example
build/fir/fir1 dis build/reals.fir build/reals.dis
build/fir/fir1_dis_example fir/v1/examples/dis/reals.as build/embedded-reals.dis as3
build/editor/disvm --jit --inspect-reals 0 3 build/reals.dis
build/editor/disvm --no-jit --inspect-reals 0 3 build/reals.dis
build/editor/disvm --jit --inspect-words 24 3 build/reals.dis
```

```text
REAL 0 -11.5
REAL 8 3.75
REAL 16 -1.25
```

Первые три восьмибайтовые ячейки — значения; ячейки со смещениями 24/32/40 —
целые флаги, все равны 1. Чтение real и words делается отдельными запусками;
эти числовые примеры детерминированы. В общем случае повторный запуск программы
с вводом/эффектами не является единым снимком. Типы в выводе автоматически не
угадываются, опции чтения диапазона взаимоисключающие. Число trace может меняться
после оптимизации: смещения надо брать из выбранного FIR.

Для встраивания main.c сначала создаёт временные FIR и Dis, а выходной файл
открывает после успешной компиляции. Все потоки закрываются на обоих путях.
Клиент IDE должен сериализовать вызовы парсера на сборочном worker и передавать
результаты через каналы; выполнение host-C примера на самой DisVM не заявляется.
Установленный SDK проверяется повторной линковкой main.c из одних поставленных
заголовков/архивов и побайтовым сравнением с CLI после переноса каталога.

## Вещественный Limbo

[real.b](real.b) — полностью комментированный профиль `value(): real`. Coco/R
проверяет совпадение типа интерфейса и определения; локальные значения остаются
real. Дробные/экспоненциальные литералы задаются явно (`2.0`, `1e2`): целые,
radix/hex-литералы и смешанные int/real отклоняются, а не неявно преобразуются.

```sh
build/fir/fir1_source limbo --input fir/v1/examples/dis/real.b build/real.fir --module dis_example
build/fir/fir1 dis build/real.fir build/real.dis
build/fir/fir1_dis_example fir/v1/examples/dis/real.b build/embedded-real.dis
build/editor/disvm --jit --inspect-reals 0 1 build/real.dis
build/editor/disvm --no-jit --inspect-reals 0 1 build/real.dis
```

```text
REAL 0 42.5
```

FIR сохраняет `source_language limbo` и новый `source_semantics limbo_real`.
Первая строка остаётся FIR 1.0; для нового профиля нужен обновлённый парсер.
Старый int-профиль по-прежнему имеет `limbo_i32`. Флаг исполнения расположен
по смещению 8 и читается `--inspect-words 8 1`. Чтения разными опциями означают
отдельные запуски, а не единый атомарный снимок.

Показаны явное real-объявление, inferred real, деление и запись во внешнее
лексическое окружение. Реальные сравнения, if/else в real-функции, смешанные
типы, casts, аргументы функций и импорты не реализованы. Обычный Limbo int
сохраняет wrap-арифметику и терминальные решения. SDK-пример использует прежний
вызов без третьего аргумента: тип берётся из грамматики, не из расширения файла.

[Full-width big CFG / Полноразрядный big CFG](../../doc/BIG_CFG.md) · [FIR](big-cfg.fir).
