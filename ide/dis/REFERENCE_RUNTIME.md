# Reference runtime caveat / Ограничение reference VM

The development bootstrap for this series is InferNode
`abb1b6860238f89eb5a10125e561eff4e0a34836`, built headless on Linux AMD64.
It is external test tooling; its source/libraries are not imported into FlashVM.

During Find regression development, after the small search cases, constructing
a shared long-string fixture as below reproducibly crashed that emulator in both
`-c0` and `-c1`:

```limbo
# Within the find_test.b init, after the sixteen small search fixtures:
repeated := "a";
while(len repeated < 32768)
    repeated += repeated;
longtext := repeated + "b";
longneedle := repeated[:16384] + "b";
search(longtext, longneedle, 0, 32769, core->Ok, 16384, 1, 16384);
search(repeated, longneedle, 0, 32768, core->NotFound, -1, -1, -1);
```

The diagnostic was `fault while holding 1 lock(s): dereference of nil (proc dis)`.
Temporary native backtrace instrumentation located `poolfree` → `destroyh` →
`destroy` → `addc` → `xec`. Adding progress printing changed the outcome; a small
standalone concatenation reproduction did not fail. This is therefore a recorded
runtime/lifetime-sensitive failure, not a completed root-cause diagnosis or proof
that all string operations are correct. The instrumentation was removed before
final acceptance. No external runtime fix is included in the patch.

The committed stress fixtures construct independent long strings through byte
arrays instead. They still exercise the actual document service and the linear
prefix matcher with 32769-character text and a 16385-character pattern, including
late match and no match. This avoids testing unrelated concatenation lifetime
behavior as part of the search algorithm contract. It does not accept the failing
concatenation scenario or general runtime ownership correctness.

The selected user experiment was inspected at
[`bfa6a601`](https://github.com/bhgv/Inferno-OS-bhgv/commit/bfa6a6019bb5414f233b66d60e219c476be298cd).
It was not built or executed for these acceptance results. Execution in that VM,
FlashVM embedding and instrumented x86-64/arm64/riscv64 JIT entry remain open.

## Русский

Внешний headless bootstrap InferNode `abb1b686` падает на приведённом варианте
большого строкового fixture после малых поисковых случаев. Native backtrace
показал путь `poolfree` → `destroyh` → `destroy` → `addc` → `xec`. Progress print
меняет результат, а короткий отдельный repro не падает: полная причина ещё не
установлена. Временная диагностика удалена до итоговых проверок; исправление VM
не входит в патч. Большие поисковые fixtures создают независимые строки через
byte arrays, сохраняя настоящий тест KMP, но не заявляя приёмку проблемной
конкатенации или всех ownership-путей runtime. Пользовательская experiment
просмотрена на `bfa6a601`, но не исполнялась; встраивание и native JIT ещё открыты.

The [0.5.0 series](SERIES_0_5_0.md) records another allocation-workload-dependent
self-concatenation failure and an optional probe; no native runtime fix is claimed.
