# Native Dis IDE workspace tutorial

Run `python3 build_menu.py`, choose `all` or `disvm-runtime`, accept defaults,
then choose **5** from the post-build launch menu. The complete runtime, matching
compiler, native modules, debug symbols and examples are installed automatically.
The independent command is `bash out/disvm-runtime/run-ide.sh`.

Every source line of `main.b` explains its role. The entry loads actual Sys,
idecore, ideworkspace and the reusable workspace tutorial. That tutorial spawns
workspace/document owners, opens a document, obtains its immutable revision-bound
build snapshot, and consumes all shutdown acknowledgements. The program reports:

```text
PASS native Dis IDE workspace: documents, build snapshot, channels and shutdown
PASS Dis application completed
```

The launcher also prints actual total/native instruction counters. A source-load,
link or service failure produces a nonzero runtime result. This example exercises
Dis-side IDE ownership and channels; graphical editor integration is still open.

The matching NC64 experimental ABI and supported Sys subset are documented in
[the runtime guide](../../../../disVM/runtime/README.md). Source headers and
compiled native modules must match. The installed classic `out/ide/modules`
package remains separate and cannot be mixed with this native runtime.
