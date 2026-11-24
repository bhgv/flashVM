# Menu 8 desktop on two VMs

[Русский](DESKTOP-PAIRED.ru.md) · [Entry points](https://github.com/bhgv/flashVM/blob/master/ide/ENTRYPOINTS.md)

The ordinary DisVM build now produces the same `desktop-ide.dis` that menu 8
uses. Its pane owners, layout, dialogs and file operations come from the
existing Limbo sources. The graphical menu chooses the installed
`run-desktop-ide.sh`, which executes `disvm-editor` to render the full desktop.

## Verified paired path

With the terminal SDK, DisVM also builds `desktop-paired.dis`. Run it with the
module directory as the working directory, a presentation `desktop-view.swf`
in that directory, and the player's endpoint mounted at `/rpc`. The Limbo
launcher uploads the movie through `/rpc/.upload`, writes `start` to
`/rpc/.state` and enters `DesktopIDE.init()` on the same owner. The real
`$Unibus.attach` then carries scene and input frames in both directions on
the existing libstyxrpc session.

The AS acceptance movie reads the scene with two editors, a build console
and Help. It inserts `Яpaired` into the first editor, waits for the next
view to contain it, drives the Save As dialog and enters `paired-output.txt`.
It waits for the saved document's caption and requests ordinary Quit.
The check requires the original desktop's acknowledged shutdown and checks the
exact bytes on disk from a fresh working directory. It runs with both
interpreter and JIT on the SDK-enabled executables.

From the repository root, configure `disVM/runtime` with the source-matched
`DVM_TERMINAL_LAUNCH_SDK`. Build the ordinary runtime and run the check.

```sh
cmake --build build/disvm-runtime --target disvm disvm_desktop_paired
python3 ide/tests/check_paired_desktop.py --disvm build/disvm-runtime \
  --player build/flashvm/flashvm_styx_player --as2 build/as2/view/as2_viewc \
  --fir build/fir --swl build/linker/swl
```

The check compiles the SWF with AS2 → FIR → SWO → SWL, copies the Limbo-built
modules into isolated working directories, and provides the two VMs with the
ends of one connected socket. Application behavior runs in the compiled Dis
modules and the SWF.

## Graphical integration still required

The paired test consumes real scene records. The standalone presentation
still needs to turn those records into the desktop's MovieClip and TextField
hierarchy, report native window/input events through UniBus, and attach the
remaining font metrics, vector bulk and ink services. Menu 8 uses the installed
graphical runtime for these features.

[Limbo launcher](https://github.com/bhgv/flashVM/blob/master/ide/dis/app/desktop-paired.b) ·
[Desktop controller](https://github.com/bhgv/flashVM/blob/master/ide/dis/app/desktop.b) ·
[ActionScript acceptance](https://github.com/bhgv/flashVM/blob/master/ide/tests/paired-desktop.as)
