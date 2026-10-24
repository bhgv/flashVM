# Пример Undo/Redo через каналы

[English](README.md) · [Контракт](../../HISTORY.ru.md) ·
[Сборка ядра](../../README.ru.md)

[main.b](main.b) — полностью комментированный Limbo-клиент настоящего владельца
документа. Каждая операция идёт по типизированным request/reply каналам.
Open first даёт ревизию 1, правка second — 2, Undo first — 3. Устаревший Redo
отклоняется; актуальный Redo даёт second/4, Undo — first/5. Новая правка third/6
удаляет Redo, а сохранённый исходный Reply всё ещё содержит first/1.
Saved остаётся 1: никакого сохранения на диск не было.

Из корня, с переменными внешнего нативного Limbo/emu/module SDK:

```sh
python3 build-menu.py --projects ide --demo history
```

Ожидается:

```text
undo text=first revision=3 saved=1
branch text=third revision=6 saved=1
PASS idecore historyexample 12 checks
```

`--tests` дополнительно запускает все проверки ядра и host IDE. После сборки
один режим можно выполнить командой
`ctest --test-dir build/workspace/ide -R '^ide_dis_history_example_mode_0$' -V`.
Проверяются exit и полная строка PASS. Sys нужен только bootstrap-примеру для
вывода и остановки своей отдельной VM. Production-ядро не импортирует Sys,
Draw/Tk. history.dis не устанавливается. Режимы reference emu исполняют настоящий
модуль, но не доказывают новый целевой JIT. Пример не является GUI или отладчиком.
