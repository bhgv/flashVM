# fvm_styxrpc_playback_create

[English](STYX_RPC_PLAYBACK_CREATE.md) · [API](../include/flashvm/styxrpc_playback.h) · [Source](../src/styxrpc/playback.c) · [Guide](STYX_RPC_MODULES.ru.md)

`flashvm_styxrpc`

```c
FVM_Result fvm_styxrpc_playback_create(const FVM_StyxRpcPlaybackSpec *spec,
    FVM_StyxRpcPlayback **out);
```

## Контракт

Передайте runtime, положительный предел байтов SWF и режим исполнения. Необязательная сессия Dis даёт исключительный файловый корень. Setup настраивает каждый новый контекст, debug подключает отладчик, statistics дописывает строки с явной длиной. Плеер копирует параметры и заимствует runtime, сессию и состояние хоста. Успех записывает пустой плеер в `out`. Ошибка проверки или выделения сохраняет `out`. Обслуживайте сервер и плеер в потоке владельца.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_playback_create` | function | [include/flashvm/styxrpc_playback.h:32](../include/flashvm/styxrpc_playback.h#L32) |
| `FVM_StyxRpcPlayback` | struct | [src/styxrpc/playback.c:30](../src/styxrpc/playback.c#L30) |
| `FVM_StyxRpcPlaybackSpec` | struct | [include/flashvm/styxrpc_playback.h:17](../include/flashvm/styxrpc_playback.h#L17) |
<!-- source-links:end -->
