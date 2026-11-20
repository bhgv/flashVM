# Owner-side scalar dispatch

[Русский](SCALAR-OWNER.ru.md) · [Header](scalar_owner.h) · [Test](tests/scalar/route.c)

The C API in `styx_terminal` lets the owner thread take one request from an
existing `.unibus` inbox. The Styx thread acknowledges the inbox write.
The owner then reads the argument, executes its method and sends the result
over the reverse route.

| Function | Contract |
| --- | --- |
| `styxscalarowner` | Borrow the exact request and reply providers with nonzero 32-bit tags. Open the inbox for reading and retain the reply provider. Return an owned object or NULL. |
| `styxscalarownertake` | Validate the tag and kind of one complete S-record. On success set the signed 64-bit argument and return `STYX_GATEWAY_VALUE`. An empty live inbox yields `STYX_GATEWAY_PENDING`. Repeated takes and untrustworthy records yield `STYX_GATEWAY_UNKNOWN`. |
| `styxscalarownersend` | Make one reply write after a successful take. `success == 1` sends a result. Any other value sends an owner error. A second reply is refused. |
| `styxscalarownerfree` | Close the inbox and release the reply provider. NULL is allowed. |

Access is serialized. I/O may block and follows the active native cancellation
scope. The native round trip uses a C method. Dis export invocation needs a
separate entry from the VM owner's thread.
