# Bulk replacement and a single Undo

[Русский](README.ru.md) · [Contract](../../REPLACE_ALL.md) · [Source](main.b)

This fully commented Limbo example loads the document service, opens
`old中 old中 old中`, replaces all three `old中` occurrences with `old中新`, rejects a
stale revision and restores the original text with one Undo. The replacement
contains the needle: the three inserted occurrences must not be searched again.
Revision advances 1 → 2 → 3; saved remains 1. Retained original replies stay intact.
Sys is used only by the developer bootstrap for reporting and isolated VM shutdown;
production idecore imports only its own header and controls its state by channels.

From the workspace root, use an explicitly selected, ABI-matched reference SDK:

```sh
LIMBO_EXECUTABLE=/matching/bin/limbo \
LIMBO_MODULE_DIR=/matching/module \
EMU_EXECUTABLE=/matching/bin/emu \
./build-workspace.sh --projects ide --demo replaceall
```

Or choose `ide` and demo `replaceall` in `python3 build-menu.py`. The root script can
build the temporary Limbo bootstrap from tools; execution still requires a matching
real emulator. Root disVM currently builds a core archive, not that executable.
Never substitute the archive path for EMU_EXECUTABLE.

For standalone builds, configure ide/dis with those same three CMake variables,
then build and run `ctest --test-dir BUILD -R replaceall_example -V`.
Both reference modes must print `PASS idecore replaceallexample ... checks`.
Test modules/examples are not installed with the production library. This is a
console service example, not a rendered editor, filesystem transaction or complete
FlashVM-hosted IDE. All client modules must use interface 0.5.0.
