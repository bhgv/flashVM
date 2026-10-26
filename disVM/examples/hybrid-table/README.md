# Executable native hybrid-table example

[Русский](README.ru.md) · [Commented program](main.b)

The hosted `$Bigtable` builtin wraps the repository's C99 hybrid scalar core.
It is available in matching `disvm` and `disvm-editor` builds. This is an explicit
module API, not yet automatic lowering of `table of big` through FIR.

```limbo
tables := load Bigtable Bigtable->PATH;
t := tables->create();
status := tables->settext(t, "total", big 42);
(readstatus, value) := tables->gettext(t, "total");
```

Always check status before using a read value. Missing is distinct from a stored
zero. Numeric keys are nonnegative `big`; strings are copied as UTF-8 with explicit
length, including embedded NUL. `length` reports the uninterrupted numeric prefix
from zero, while `count` includes every numeric/string entry.

`slice` copies a checked half-open numeric range into an independent table.
`keysof` produces an independent immutable key owner, indexed using `keyat` and
sized using `keylen`. Each key is returned as `(kind, numeric, text)`; only the
payload selected by kind is meaningful. Keys survive source table destruction.

Handles are GC-managed native owners. Their public reserved field carries no
pointer. Fabricated public ADTs are rejected by exact native type identity.
The finalizer releases copied C storage; it contains no cross-heap VM references.
Calls run synchronously on the current sequential Dis scheduler owner. They are
not a network protocol, shared multithreaded object, or remote capability.

Build the runtime, then:

```sh
ctest --test-dir build/runtime -R application_hybridtable --output-on-failure
cd build/runtime
./disvm --stats --jit hybridtable-test.dis
./disvm --stats --no-jit hybridtable-test.dis
```

The general Coco/R Limbo frontend accepts the extension syntax. Separately, our
fir1_source big-result subset now performs typed lowering of table creation,
numeric/string/dotted access, slices, keysof and len into FIR. See the
[compiler contract](../../../fir/v1/doc/TABLES.md). Its hosted C reference runner
is distinct from this native Dis module example. Automatic compiler-generated
native Dis/SWF table calls, keyseq indexing, pair iteration and typeof/any remain
pending; the module example continues to use the matching bootstrap explicitly.
