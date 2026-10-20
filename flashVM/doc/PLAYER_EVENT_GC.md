# Player collection between frames

Since v0.667 each foreground main-loop turn calls [`fvm_gc_step`](../include/flashvm/gc.h#L90) after input
delivery, HostInput publication and timer polling. Previously this safe point
required a fired timer, leaving temporary input-handler objects until the next
slow timeline frame.

This checks the collector threshold rather than forcing a full collection.
Collector tuning and suspension remain effective. The additional call does not
advance the timeline, fire a timer or request a presentation. It runs on the VM
thread after handlers return, with the scene root and HostInput still pinned.
The existing post-frame step remains for frame-action allocations. The
background branch gains no input dispatch or collection.

The regression uses the actual player, VM, rasterizer and collector with a
protocol clock/window. Between frames at 0 and 500 ms, the input source creates
more than the allocation threshold of temporary objects. It requires actual
reclamation at 100, 200, 300 and 400 ms, exactly two frames and two presentations,
and no timers. Interpreter and JIT are checked separately. Restoring the old
condition fails both modes. This is not device, physical-window or endurance
acceptance.

```sh
cmake --build build-player --target flashvm_frontend_event_gc
ctest --test-dir build-player -R player_event_gc --output-on-failure
```

[Player source](../frontend/common/player.c) ·
[Collector fixture](../frontend/tests/event_gc.c) ·
[Acceptance checks](../frontend/tests/event_gc.py) ·
[Build instructions](DESKTOP_FRONTEND.md) ·
[Roadmap status](ROADMAP_STATUS.md)
## C entry and arguments (v0.668)

`fvm_desktop_main` validates positive argc, non-null argv and every argc pointer,
and guards the path-array allocation size. Invalid vectors return 2 before the
array allocation, file reads or SDL/VM startup. Null option operands and null
entries after `--help` also fail. No `argv[argc]` sentinel is read. On failure,
the parser leaves its result structure unchanged; the path array is scratch
storage and has no such guarantee. The host must supply an accessible array
and NUL-terminated strings: pointer checks do not validate memory mappings or
arbitrary addresses. Calls are synchronous and the strings remain host-owned.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_gc_step` | function | [include/flashvm/gc.h:90](../include/flashvm/gc.h#L90) |
<!-- source-links:end -->
