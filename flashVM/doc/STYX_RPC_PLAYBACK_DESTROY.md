# fvm_styxrpc_playback_destroy

[Русский](STYX_RPC_PLAYBACK_DESTROY.ru.md) · [API](../include/flashvm/styxrpc_playback.h) · [Source](../src/styxrpc/playback.c) · [Guide](STYX_RPC_MODULES.md)

`flashvm_styxrpc`

```c
void fvm_styxrpc_playback_destroy(FVM_StyxRpcPlayback *playback);
```

## Contract

Release the movie context, retained upload bytes and control descriptor. Pass the owned playback after closing all using servers or NULL. Run on the owner before releasing the borrowed runtime and session.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_styxrpc_playback_destroy` | function | [include/flashvm/styxrpc_playback.h:47](../include/flashvm/styxrpc_playback.h#L47) |
| `FVM_StyxRpcPlayback` | struct | [src/styxrpc/playback.c:37](../src/styxrpc/playback.c#L37) |
<!-- source-links:end -->
