# Scalar calls over Styx Unibus

[Русский](SCALAR-ROUTE.ru.md) · [API](scalar_route.h) · [Example](tests/scalar/route.c)

## Send a request and return its result

`scalar_route.h` · `styx_terminal`

The host supplies two authorized writable `.unibus` files, one published by
each peer. The caller sends an S-record with one signed 64-bit argument using
`styxscalarcallfile(request, 17, argument)`. The owner's write handler passes
the complete record to `styxscalarreplyfile(reply, 17, bytes, count, 29,
method, state)`. The callback returns 1 and sets the result, or returns
another value and sets the error code. The caller decodes the separate reply
using `styxscalarread(29, ...)`.

Both operations borrow the file provider and open a fresh write fid for one
record. The delivery status describes that write. The owner may have executed
the method by the time a reply write fails. Treat uncertain delivery as an
unknown outcome and never resubmit automatically. The host serializes calls
on each pair of routes and allows one outstanding call.

## Run the example

From the configured native SDK build directory with VFS support enabled:

```sh
cmake --build . --target terminal-scalar-route
ctest -R '^terminal-scalar-route$' --output-on-failure
```

The example runs two Styx sessions over one bidirectional carrier. It checks
a result, a method error and rejection of a request with the wrong route tag.
