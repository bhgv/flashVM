# Inspect a Dis64 call image without executing it

`check.py` is a commented client of `inspect_image(bytes)`. It reads one explicit
binary/hex file under a size ceiling and prints stages, inferred role/flow facts
or a refusal. It creates no runtime object, output image, native pointer or JIT
mapping. It does not use a private manifest/local-kind table.

Checkout example with the exchanged Inferno corpus:

```sh
python3 disVM/envelope/examples/typed-checkpoint/check.py /path/to/goldens/answer42.hex
```

Installed runtime/editor SDK root:

```sh
python3 examples/dis64-envelope/typed-checkpoint/check.py /path/to/goldens/answer42.hex
```

For that exact immutable image, expect `bounded-checks-passed`, the inferred
root/helper facts, and `owning_execution_supported:false`. Other images can be
unsupported or refused. Exit0 means the inspection completed; it never means
execution is permitted. Input I/O/hex/ceiling errors produce a diagnostic.

To inspect a complete identity-pinned corpus use
`python3 source/dis64-checkpoint/peer_call_semantics.py /path/to/goldens` in the
SDK. Only names/byte lengths/SHA identities are read from the manifest. Private
accept/result/locals/policy fields are inert. The full self-contained synthetic
suite runs with `python3 source/dis64-checkpoint/test_peer_call_semantics.py`.

The guide is at `docs/dis64/TYPED-CHECKPOINT.md` and the archived per-image report
at `docs/dis64/typed-checkpoint.json` in the SDK. Bool/ModuleData64/managed frames/
CFG/import resolution and owning allocation/roots/GC/frame/code lifetimes remain
outside the bounded scalar proof. No shared wire ID or launch default changes.
