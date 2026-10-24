# Reproduce the GC comparison / Повторение сравнения GC

[RU report](../../docs/GC-COMPARISON.ru.md) · [EN report](../../docs/GC-COMPARISON.md)

Use a current trusted Release FlashVM build with `flashvm_reference_swf` enabled.
The scripts read only its static-library link list; they do not evaluate shell
commands from link.txt, download sources or change a compiler/runtime policy.
The native library must match the audited GC sources; source hashes in the result
do not prove that an unrelated/stale binary was built from them.

```sh
./build-workspace.sh --projects vm
python3 tools/gc-compare/run.py --flashvm-build build/workspace/vm --output build/gc-comparison
python3 tools/gc-compare/checks.py --flashvm-build build/workspace/vm --output build/gc-comparison/checks
python3 tools/gc-compare/audit_inferno.py --inferno-source /path/to/Inferno-OS-bhgv --output build/gc-comparison/inferno-layout.json
```

`--counts 10000,50000 --repeats 5` are the benchmark defaults. Every sample is a new
process/context; graph reclamation and string retention are checked. C clock()
measures process CPU around the collector. The measurement wrapper has exactly
one child, so its ru_maxrss is not polluted by earlier compiler/test children.
Peak RSS is whole-process memory (KiB on Linux, bytes on macOS), not live heap.
Windows resource counters and non-GNU/static-library link layouts require adapters;
actual acceptance here is Linux with C89/C++98 compilers and an owned FlashVM build.

The allocator audit reads exact committed owner blobs via git show at the pinned
Dis revision. `--ref COMMIT` explicitly audits a newer/fixed allocator against the
selected headers. PASS_LAYOUT_ONLY is not proof of allocator or VM correctness.
The Inferno deterministic-pool regression is deliberately not used for timing.
Generated JSON/logs belong under build output and are not committed source.

Русский: используйте актуальную Release-сборку FlashVM с reference runner.
Библиотека должна соответствовать исходникам GC. Скрипты не исполняют shell-строки
из link.txt и не скачивают зависимости. Каждый замер — новый процесс; проверяются
освобождение графа и удержание строк. Время — CPU коллектора, RSS — максимум всего
процесса, не объём живой кучи. Windows и другие форматы линковки ещё требуют
измерительных адаптеров. Аудит Inferno читает точный commit через git show;
`--ref` выбирает исправленный allocator явно. PASS_LAYOUT_ONLY не равен приёмке VM.
Тестовый пул Inferno не используется для сравнения скорости. Результаты JSON/logs
хранятся вне исходников, в build.
