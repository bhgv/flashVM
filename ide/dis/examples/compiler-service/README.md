# Revision-bound native compiler service tutorial

[Русский](README.ru.md) · [Complete service contract](../../../COMPILER-SERVICE.md) · [Fully commented source](main.b)

The actual Dis entry opens a canonical logical document, obtains its real
revision, sends an optimized Limbo→Dis build through idecompiler, fetches the
admitted independent artifact, then acknowledges both worker and workspace
shutdown. Every nonblank source line is commented. It loads no UI/Draw/Tk module,
spawns no host compiler process and does not automatically execute the output.
The source computes 6×7; native acceptance separately executes compiled artifacts.

From an installed enabled package, run in its directory or use absolute paths:

```sh
./disvm --jit --stats --quantum 1 ./compiler-service.dis
./disvm --no-jit --stats --quantum 1 ./compiler-service.dis
```

Expected marker: PASS compiler tutorial: revision=ok artifact=ok shutdown=ok.
Fircompiler=ON must be recorded in NATIVE-MODULES.txt. OFF packages keep the
portable worker but its start returns nil; the tutorial cannot fabricate a build.
The native call is serialized/blocking and has no cancellation. GUI panels and
final debugger PC maps are not implemented by this example.

compiler-service-check.dis is separate acceptance code: it builds 24 AS1/2/3/
Limbo artifacts, saves them to the caller cwd through explicit Sys.create, and
checks diagnostics, lifetimes, real workspace failures and stale results. Run it
in an empty scratch directory. check.py is developer tooling, comparing those
bytes/debug with the independent CLI and executing Dis/SWF. It is not required
by the deployed worker. client.c relinks the independent C99 service and tests
24 successes, 32 clean failures, recovery and repeated disposal.

The public worker status is a workspace result; inspect its compiler stage too.
Only admitted success returns debug. Fetch the artifact through the workspace
owner rather than retaining native pointers or confusing a failed newer build
with the previous successful artifact.
