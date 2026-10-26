# IDE application on our DisVM

[Русский](README.ru.md) · [Commented application entry](main.b) ·
[Typed controller interface](ide.m) · [Controller and gestures](../examples/graphical-editor/README.md) ·
[Runtime](../../../disVM/runtime/README.md).

`ide.dis` is a real Limbo application entry. It loads the GraphicalEditor module
through a checked interface and calls its controller on the same native DisVM
scheduler/GC. The loaded module stays rooted on the caller stack until shutdown.
Missing or incompatible modules fail explicitly. No external Inferno emu, Python
or Node process runs the deployed application.

The controller's document/history, input, viewport, modal field and file policy
remain Limbo/Dis. Copied typed channels connect that policy to a separately owned
FlashVM renderer. The application entry performs no direct UI access. The AS
presentation worker is still a separate implementation, not executed by this host.

## Build and open a document

From the repository root:

```sh
./build-workspace.sh --projects fir,disvm-editor --jobs 4
sh out/disvm-editor/run-ide.sh --jit --new example.b
sh out/disvm-editor/run-ide.sh --no-jit --document existing.b
```

Direct CMake builds can run the same entry without installation:

```sh
build/runtime/disvm-editor --jit --new example.b build/runtime/ide.dis
```

The installed graphical package resolves modules, font and license beside its
launcher, preserving the caller cwd for ordinary document paths. Noto Sans Mono
starts at 11 pixels. Ctrl+wheel/pinch change scale through checked events; line
numbers can be hidden. The console-only package retains its existing run-ide.sh;
the graphical package also ships run-ide-console.sh for that console client.
The older run-graphical-editor.sh stays available for the standalone controller.

## Usable and pending parts

Implemented and exercised: Unicode editing, cursor/selection, Undo/Redo, file
open/save/save-as, dirty-close refusal and explicit discard, find/replace, viewport,
zoom, resize, gutter and live document projection inspector. The graphical entry
is tested with actual Dis bytecode and FlashVM raster output in both VM modes.
SDL replay uses a dummy video driver; it does not establish physical touch/desktop
acceptance. Interpreter fallback is not reported as execution of a native opcode.

Our source compiler/FIR libraries are callable independently by CLI and C99 API;
see [table-language tutorial](../../../fir/v1/doc/TABLE_MUTATION.md). The current
Coco/R Limbo frontend is bounded and does not compile the whole controller yet:
a matching source-built Limbo compiler bootstraps these IDE modules. The deployed
modules themselves execute on our own DisVM. The Build/Run/Debug GUI panels,
full Dis debugger connection and complete self-hosted Limbo compilation remain
unfinished. Shipping an application entry is not a claim that those panels exist.

## Verification and diagnosis

The `disvm_ide_application_jit/interpreter` checks run this entry through the
existing full editing/file/history/raster scenario. Relocated SDK acceptance also
opens/closes ide.dis in both modes with shipped modules and default font. Exact
renderer snapshots and files are checked; no successful dummy launcher is used.
Both `.dis` and `.sbl` are shipped. If loading fails, rebuild all modules from the
same interfaces and check the entry/module directory rather than installing emu.
Large million-character replace-all has a separate 90-second test budget after
operand safety checks; its data and functional oracles are unchanged.
