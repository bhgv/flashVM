# styxclientremove

[Русский](styxclientremove.ru.md) · [Header](../../client/client.h) · [Source](../../client/clunk.c)

Link `Styx::client`. Pass an owned fid and optional transfer result storage.
The function sends one `Tremove` and consumes ownership of the fid on
every outcome. It returns one only on `Rremove`. A lost reply can leave
the removal uncertain. Do not reuse the fid. A best-effort clunk is
scheduled if server-side retirement could not be confirmed. Serialize
this consuming operation with all other uses of the fid.
