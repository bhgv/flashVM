# fvm_styxrpc_playback_tick

[Русский](STYX_RPC_PLAYBACK_TICK.ru.md) · [API](../include/flashvm/styxrpc_playback.h) · [Source](../src/styxrpc/playback.c) · [Guide](STYX_RPC_MODULES.md)

`flashvm_styxrpc`

```c
FVM_Result fvm_styxrpc_playback_tick(FVM_StyxRpcPlayback *playback, unsigned int ticks);
```

## Contract

Use the owner event loop to advance the requested number of timeline ticks. Zero ticks or non-running playback return [`FVM_OK`](../include/flashvm/flashvm.h#L44) without advancing. NULL returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). An execution failure returns its VM status and records `failed`. Call between Styx dispatches on the same owner thread. Tick can execute ActionScript synchronously.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_playback_tick` | function | [include/flashvm/styxrpc_playback.h:42](../include/flashvm/styxrpc_playback.h#L42) |
| `FVM_StyxRpcPlayback` | struct | [src/styxrpc/playback.c:37](../src/styxrpc/playback.c#L37) |
<!-- source-links:end -->
