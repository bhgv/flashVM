# Token-query example

[Русский](README.ru.md) · [Library contract](../../../TOKEN-QUERIES.md) ·
[Lazy analyzer](../../../SYNTAX-LAZY.md) · [Fully commented source](main.b)

This is a runnable Limbo program, not a GUI mockup. It obtains an actual Coco/R
ActionScript catalog, observes the `alpha` token, prints sorted lexical names,
finds a later occurrence outside strings/comments, then analyzes a changed local
snapshot and reports reuse. It never edits a file or document selection.

Requirements: the source-built matching NC64 `disvm`, shipped `idesyntax.dis`,
`idetokenquery.dis`, `limbolex.dis` and `aslex.dis`. The temporary native bootstrap
can rebuild the example, but is not the project's primary compiler. No Draw/Tk,
external emu, font, window, Python or generator is needed during execution.

Build/install via root orchestration, then choose menu17 or run:

```sh
python3 build-menu.py --projects disvm-runtime --backend cmake --launch tokens
sh out/disvm-runtime/run-token-query.sh --no-jit
sh out/disvm-runtime/run-token-query.sh --jit
```

Inside a relocated installed runtime/editor directory, rebuild the shipped source
using only the matching package's compiler and headers:

```sh
./limbo-native -I include/limbo -o tokenquery-local.dis examples/token-query/main.b
./disvm --no-jit tokenquery-local.dis
./disvm --jit tokenquery-local.dis
```

Expected output includes `Context: alpha [4,9)`, candidates `alpha` / `alpine`,
the later `alpha` match, actual `scanned`/`reused`/`restart` counters and a final
`PASS token-query example`. Counters are record observations, not a benchmark.
The example contains one local literal change; it checks that the first snapshot
retains its original value. Every successful worker start reaches an acknowledged
stop before status failure or success is reported. Requests/results rendezvous;
these waits are deliberate in this console tutorial. The GUI uses a lazy owner.

| API | Use and ownership |
| --- | --- |
| Idesyntax.start | Create one private scanner worker and two owned rendezvous capabilities |
| Idesyntax.Request | Submit immutable source/correlation values; one receive per accepted job, including stop |
| Idetokenquery.at | Borrow a completed catalog, return one scalar token value without changing selection |
| Idetokenquery.complete | Borrow exact source/catalog, return a receiver-owned bounded sorted array of lexical names |
| Idetokenquery.find | Borrow exact source/catalog, return the first literal coordinates admitted by the class mask |

All queries require matching revision/profile and exact source; arrays must not
be mutated concurrently. Native/VM pointers never cross these channels. Lexical
names are not semantic completion: there is no type/scope/import resolution.
Profile/resource/Unicode and class5 whitespace/comment limitations are documented
in the linked library contract. This example does not execute new-format Dis64
or prove a native Frame64 calling ABI or physical UI latency.
