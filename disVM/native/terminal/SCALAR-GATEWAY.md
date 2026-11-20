# Native scalar gateway

[Русский](SCALAR-GATEWAY.ru.md) · [Header](scalar_gateway.h) · [Styx test](tests/scalar/route.c)

The C gateway in `styx_terminal` takes three exact file grants. The profile
and reply files are read-only, while the request file is opened for writing
only after the profile matches the expected compiled import.

| Function | Contract |
| --- | --- |
| `styxscalargateway` | Compare one profile record with the supplied name, signature and tag. On success retain the request provider and open the reply. Return an owned gateway or NULL. |
| `styxscalargatewaysend` | Attempt one signed 64-bit argument on the configured request tag. Return the existing delivery status. Every subsequent send is refused. |
| `styxscalargatewayreceive` | Read a complete reply on the configured reply tag. Return pending on an empty pending read, value or owner error on success, and unknown otherwise. Only value and owner error change the output. |
| `styxscalargatewayfree` | Release the request provider and close the reply cursor. NULL is allowed. |

Use fresh one-shot files for each call and serialize access to each gateway.
Provider I/O can block and follows the active native cancellation scope.
The host associates all three grants with the same publication. This SDK
API does not yet connect the Dis loader or interpreter to the gateway.

Run `ctest -R '^terminal-scalar-route$' --output-on-failure` from the native
SDK build directory. The test checks a profile mismatch, explicit owner
errors and discarded duplicate sends over live Styx routes.
