# fvm_styxrpc_playback_control

[English](STYX_RPC_PLAYBACK_CONTROL.md) · [API](../include/flashvm/styxrpc_playback.h) · [Source](../src/styxrpc/playback.c) · [Guide](STYX_RPC_MODULES.ru.md)

`flashvm_styxrpc`

```c
StyxRpcControl *fvm_styxrpc_playback_control(FVM_StyxRpcPlayback *playback);
```

## Контракт

Вернуть заимствованный описатель для `styxrpc_control_tree` или необязательного поля смешанного модуля. Для NULL возвращается NULL. Используйте дерево до уничтожения плеера после закрытия сервера.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_StyxRpcPlayback` | struct | [src/styxrpc/playback.c:33](../src/styxrpc/playback.c#L33) |
<!-- source-links:end -->
