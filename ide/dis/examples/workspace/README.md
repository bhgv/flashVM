# Workspace coordination on Limbo / Рабочее пространство на Limbo

[Every-line source](main.b) · [Module header](main.m) · [Protocol](../../../WORKSPACE.md)

This callable module takes already loaded `Idecore` and `Ideworkspace` modules.
Its `run` export opens source text, captures a revision-bound build ticket, checks
the snapshot and acknowledges shutdown of all private owners. It returns one
on success, zero on failure. It imports no Sys, Draw, Tk or graphical toolkit.
Every source line is commented, including initialization of unused scalar fields.

The root IDE build produces `workspace-example.dis` plus `.sbl` and installs them
beside `ideworkspace.dis` and `idecore.dis`. A Dis caller includes the three
headers in that order, loads those modules in its host namespace and calls:

```limbo
ok := example->run(core, workspace); # Borrow the two loaded service modules and check the returned status.
```

This is a library example, not a console program with an `init` entry point.
A real compiler worker must consume a BeginBuild snapshot and submit a result;
this tutorial deliberately creates no invented output binary. The automated
reference regression loads and runs this exact example before exercising the
larger workspace contracts. A complete executable embedded Dis host is still
under development; copying a FlashVM SWF player does not make it one.

Пример демонстрирует настоящие блокирующие каналы Dis, владение документом,
билет сборки и подтверждённую остановку. Весь код подробно прокомментирован.
В нём нет имитации компиляции, скрытого Sys/GUI или зависимости от файловой
системы программы: загруженные модули передаются параметрами.
