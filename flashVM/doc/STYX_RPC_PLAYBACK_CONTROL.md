# fvm_styxrpc_playback_control

[Русский](STYX_RPC_PLAYBACK_CONTROL.ru.md) · [API](../include/flashvm/styxrpc_playback.h) · [Source](../src/styxrpc/playback.c) · [Guide](STYX_RPC_MODULES.md)

`flashvm_styxrpc`

```c
StyxRpcControl *fvm_styxrpc_playback_control(FVM_StyxRpcPlayback *playback);
```

## Contract

Return the borrowed control descriptor for `styxrpc_control_tree` or the optional control field of a mixed module. NULL playback returns NULL. The tree may be used until playback destruction, after server retirement.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_StyxRpcPlayback` | struct | [src/styxrpc/playback_internal.h:22](../src/styxrpc/playback_internal.h#L22) |
<!-- source-links:end -->
