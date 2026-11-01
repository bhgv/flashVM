# Scalar Limbo → Dis64 example / Пример скалярного Limbo → Dis64

This fully commented example uses the temporary source-built compiler's explicit
`-6` backend. Our Coco/R → FIR compiler remains primary. The program has one
parameterless entry, no imports, managed objects or shell context. It checks
4097 loop iterations, a Big result and a Big local preserved across WORD work.
A failed check raises a real divide error.

From the installed `limbo-bootstrap` package:

```sh
./limbo-bootstrap -6 -o loop.dis64 examples/dis64-scalar/loop.b
../disvm/dis64inspect loop.dis64 --pilot --rewrite checked.dis64
```

Set the inspector path to the package you actually installed (`disvm`,
`disvm-runtime` or `disvm-editor`). Inspection and exact rewrite do not execute
this file. The report intentionally retains `execution_supported=false`.
Execution requires a separately configured experimental Inferno scalar console;
ordinary FlashVM startup is not enabled by this example. See the upstream guide
included as `UPSTREAM-SCALAR.md` for those explicit emulator commands.

The reader checks the existing module/code/types/data/string sections and scalar
roles. No new format IDs, native file calls or general managed ABI are enabled.
Without `-6`, ordinary Dis remains the compiler's default output.

Пример подробно прокомментирован: один вход без параметров, 4097 проходов цикла,
результат Big и проверка сохранности локальной Big. Нет Sys/Draw, управляемых
объектов или контекста shell. Ключ `-6` выбирается явно; по умолчанию остаётся
обычный Dis. Основным компилятором проекта остаётся наш Coco/R → FIR.

В командах выше задайте путь к установленному inspector. Проверка и точная
перезапись не исполняют программу. `execution_supported=false` сохраняется.
Для исполнения нужен отдельно настроенный экспериментальный scalar console
Inferno. Новые IDs формата и native-вызовы из файлов пример не включает.
