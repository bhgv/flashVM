# Native ink: readable C/Dis examples

For ordinary drawing, build the graphical runtime, run build-menu item 8 and choose
Files → SVG. Mouse/finger motion remains native until release. No extra utility
or Python process runs in the deployed IDE.

The C example installed here as `stamp-check.c` is
[test_ink_primitives.c](../../../../disVM/runtime/test_ink_primitives.c). It creates
an immutable `FVM_Path` and an exclusively owned RGBA surface, calls the native
stamp, checks actual pixels and demonstrates native buffer freeze/publication.
The implementation is in [editor_stamp.c](../../../../disVM/runtime/editor_stamp.c)
and its documented [header](../../../../disVM/runtime/editor_stamp.h). It does not
construct a FlashVM/Dis context or retain a managed object.

The Limbo example installed as `worker-check.b` is
[native_ink_test.b](../../tests/native_ink_test.b). It uses real document channels,
queues two frozen native tokens, waits through Save, and verifies whole-stroke
Undo/Redo and refusal of an invalid token. It requires the development fixture
runner [test_ink_runner.c](../../../../disVM/runtime/test_ink_runner.c), which seeds
native buffers before calling the real runtime. That fixture runner is not an
alternative IDE entry point and is not installed as the player.

```sh
cmake --build build/workspace/disvm-editor --target disvm_ink_primitives_test disvm_native_ink_runner
build/workspace/disvm-editor/disvm_ink_primitives_test
ctest --test-dir build/workspace/disvm-editor -R 'native_ink' --output-on-failure
```

`test_native_ink.py` independently parses the resulting SVG files in interpreter
and JIT modes. `test_live_ink.c` additionally drives 600 actual SDL motions through
the production input owner; only Begin/token End are proposed to Dis. This native
input/raster check names its copied scene boundary; it does not mock the separate
real Limbo-worker integration test.

The C/Dis interface is [idehost.m](../../bridge/idehost.m). The ordered worker is
[ideinkworker.b](../../bridge/ideinkworker.b). See the complete
[lifetime/scheduling contract](../../../NATIVE-INK.md), including capacity,
publication fences, approximate preview quality and matching runtime signatures.
