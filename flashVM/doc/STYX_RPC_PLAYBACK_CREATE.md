# fvm_styxrpc_playback_create

[Русский](STYX_RPC_PLAYBACK_CREATE.ru.md) · [API](../include/flashvm/styxrpc_playback.h) · [Source](../src/styxrpc/playback.c) · [Guide](STYX_RPC_MODULES.md)

`flashvm_styxrpc`

```c
FVM_Result fvm_styxrpc_playback_create(const FVM_StyxRpcPlaybackSpec *spec,
    FVM_StyxRpcPlayback **out);
```

## Contract

Pass the runtime, a positive SWF byte limit and an execution mode. An optional attached Dis session provides the exclusive file root. Setup runs on each fresh context, debug supplies a debugger attach hook, and statistics can append counted lines to a snapshot. The playback copies settings and borrows the runtime, session and host state. On success it writes an empty playback owner to `out`. Validation or allocation errors preserve `out`. Keep server and playback operations on the owner thread.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_playback_create` | function | [include/flashvm/styxrpc_playback.h:32](../include/flashvm/styxrpc_playback.h#L32) |
| `FVM_StyxRpcPlayback` | struct | [src/styxrpc/playback_internal.h:22](../src/styxrpc/playback_internal.h#L22) |
| `FVM_StyxRpcPlaybackSpec` | struct | [include/flashvm/styxrpc_playback.h:17](../include/flashvm/styxrpc_playback.h#L17) |
<!-- source-links:end -->
