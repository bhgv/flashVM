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


After receiving the last reply, the caller host may use
`styxscalarclosefile(request, tag)`. It sends one end S-record with the
request tag. An owner running with count zero exits on that record or EOF.
On carrier shutdown, a mailbox drains a confirmed record before reporting
EOF. Waiting for the next owner request can outlive the caller's three-second
reply budget.

The two-VM test also aborts TCP after a confirmed value, with no end record.
It checks that the owner exits its pending read and both runtimes complete
with no live file handles, pending closes or active dispatch claims.
