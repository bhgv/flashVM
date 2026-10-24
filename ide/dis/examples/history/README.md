# Channel-only Undo/Redo example

[Русский](README.ru.md) · [History contract](../../HISTORY.md) ·
[Core build](../../README.md)

[main.b](main.b) is a fully commented Limbo client of the actual document owner.
It uses typed request/reply channels for every operation. Open first at revision 1,
edit second at 2, Undo first at 3, reject a stale Redo, Redo second at 4, Undo at 5,
then branch to third at 6. Redo now returns NotFound; the retained original reply
still contains first at revision 1. Saved remains 1: no filesystem save occurred.

With the external native Limbo/emu/module SDK variables set, run from root:

```sh
python3 build-menu.py --projects ide --demo history
```

Expected output includes:

```text
undo text=first revision=3 saved=1
branch text=third revision=6 saved=1
PASS idecore historyexample 12 checks
```

`--tests` additionally runs all core checks and host IDE tests. To run only one
reference mode after building, use
`ctest --test-dir build/workspace/ide -R '^ide_dis_history_example_mode_0$' -V`.
CTest checks both process status and a complete PASS marker. Sys is imported only
by this developer bootstrap for reporting and halting its private emulator; the
product core imports neither Sys nor Draw/Tk. history.dis is not installed.
Both emulator modes execute the real module but do not prove target JIT entry.
The example is not a graphical editor, persistence service or debugger.
