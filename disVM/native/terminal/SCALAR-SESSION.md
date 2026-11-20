# Sequential scalar call session

[Русский](SCALAR-SESSION.ru.md) · [Header](unibus_server.h) · [Run two DisVMs](../../examples/remote-scalar/STYX.md)

`styxunibusserverstartsession(channel, tag, original, profile)` publishes
the files of `styxunibusserverstartmodule`. Its `.unibus` inbox accepts
another record after the previous record is consumed. A full slot refuses
writes. A read with insufficient capacity refuses and leaves the record ready.

The host grants one exclusive pair to a sequence of calls. The owner retains
one loaded module until that sequence ends. A caller's native module entry
allows one outstanding call. A verified value or owner error permits the next
call. Without a trustworthy reply, the entry stops accepting requests.
Recovery requires a fresh pair of files. Old replies remain on the old route.
Independent concurrent module instances cannot share one route in this mode.
