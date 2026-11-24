# fvm_styxrpc_playback_tick

[English](STYX_RPC_PLAYBACK_TICK.md) · [API](../include/flashvm/styxrpc_playback.h) · [Source](../src/styxrpc/playback.c) · [Guide](STYX_RPC_MODULES.ru.md)

`flashvm_styxrpc`

```c
FVM_Result fvm_styxrpc_playback_tick(FVM_StyxRpcPlayback *playback, unsigned int ticks);
```

## Контракт

Передайте число тактов из цикла владельца. Ноль тактов или остановленный плеер возвращают [`FVM_OK`](../include/flashvm/flashvm.h#L44) без продвижения. NULL даёт [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Сбой исполнения возвращает статус VM и состояние `failed`. Вызывайте между запросами Styx в том же потоке. Такт может синхронно исполнить ActionScript.

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
| `FVM_StyxRpcPlayback` | struct | [src/styxrpc/playback.c:30](../src/styxrpc/playback.c#L30) |
<!-- source-links:end -->
