# Live document projection example

[Contract](../../LIVE-PROJECTIONS.md) · [Русский](README.ru.md)

`main.b` is a fully commented executable example and regression. It starts an
actual `Idecore` owner, opens Unicode source and wraps that same endpoint with
`Idedocprojection`. The adapter does not store a duplicate document or access
its private ADT. All reads and edits use ordinary typed Dis channels.

The example checks a copied text value, bounded metadata table, a stale Undo
request, invocation consent, a bound real Undo, a copied text assignment,
cooperative adapter pause and a document larger than the scalar copy quota.
It then releases only the adapter and proves that the document remains usable
before explicitly shutting down the document owner. A real source revision is
required for every mutation; no timed-out invocation is automatically retried.

Build using the native runtime guide, then run:

```sh
ctest --test-dir build/runtime -R application_docprojection --output-on-failure
cd build/runtime
./disvm --stats --no-jit docprojection-test.dis
./disvm --stats --jit docprojection-test.dis
```

The CTest wrapper checks actual native/interpreter dispatch counters. The matching
bootstrap compiles these ADT/channel modules; our numeric FIR frontend does not
yet lower them. Classic and NC64 binaries must not be mixed.

The installed runtime package includes this source, matching headers and all
four dependent modules. Its relocated SDK test recompiles the source and runs
it in both modes from a directory with spaces. This is a local Dis-process
boundary, not network RPC or arbitrary FlashVM object reflection.

Typed helper functions readint/readtext return (status, typed value, revision). They select the requested owner conversion explicitly, reject incompatible values and retain the revision for optimistic writes. The example also checks that an earlier copied table stays unchanged after another client edits the live document.
