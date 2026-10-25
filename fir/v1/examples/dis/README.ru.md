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

Подмножество: один блок, i32-константы/copy/wrap-арифметика, захват trace и
return. Нет импортов, экспортированного Limbo ABI `value`, строк, исходных
каналов и Dis SBL. Возвращаемое значение учебного исходника становится trace
FIR `@main`; без `--inspect-words` оно не печатается. Полная графическая IDE
пока не компилируется этим подмножеством.

В установленном FIR-пакете можно заново собрать C99-клиент только из его SDK:

```sh
cc -std=c99 -I include examples/dis/main.c lib/libfir1_ir.a lib/libfir1_source_api.a lib/libfir1_io.a -o dis-client
./dis-client examples/dis/main.b answer.dis
```
