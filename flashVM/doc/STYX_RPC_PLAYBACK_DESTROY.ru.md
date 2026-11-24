# fvm_styxrpc_playback_destroy

[English](STYX_RPC_PLAYBACK_DESTROY.md) · [API](../include/flashvm/styxrpc_playback.h) · [Source](../src/styxrpc/playback.c) · [Guide](STYX_RPC_MODULES.ru.md)

`flashvm_styxrpc`

```c
void fvm_styxrpc_playback_destroy(FVM_StyxRpcPlayback *playback);
```

## Контракт

Освободить контекст фильма, сохранённые байты загрузки и описатель управления. Передайте свой плеер после закрытия всех его серверов или NULL. Вызывайте у владельца перед освобождением заимствованных runtime и сессии.

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
