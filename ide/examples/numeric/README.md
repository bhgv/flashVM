# Numeric build example

[Русский](README.ru.md). This executable AS2 numeric-subset source prints `42`.
It demonstrates a real compiler/linker input, not a complete AS2 program profile.
The [build backend guide](../../README.md) documents stage, ownership, timeout,
JSON and publication contracts. Supply trusted host executables explicitly:

```sh
python3 ide/build.py ide/examples/numeric/program.as movie.swf --language as2 \
  --compiler /host/fir1_source --ir /host/fir1 --linker /host/swl \
  --module example.numeric --runner /host/flashvm_runner --expect-trace '42
'
```

The quoted expectation contains a real newline. Success returns JSON with
ok=true and published=true, exit 0, and replaces movie.swf. Failed source,
verification, optimization, object/link/run stages preserve the previous movie.
The editor owns returned JSON and the output path; the backend reaps direct
children and removes intermediates. No VM or compiler object is retained.
