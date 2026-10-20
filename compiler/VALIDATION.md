# Current parser validation, series 0.5.0

See [current evidence](PARSER_COVERAGE.md). The following report is historical;
its sanitizer/VM/IDE results are not a fresh 0.5.0 run.

# Syntax series validation

2026-10-20, Linux C99; [Русский](VALIDATION.ru.md).

- Baseline: GitHub master `39921ad099fc74253b9f6c9d39b72687a520de34`.
- External bhgv/coco-r-C revision `b91b65233c264d92c63a5c8cd855e7fc784bca53`;
  no generated parser/runtime is included in the patch.
- Native and sanitizer builds: 22 CTest tests each (16 compiler, 3 FIR,
  2 linker, 1 combined IDE host), all passed.
- FIR suite: 732 subprocess checks per build, with real FlashVM execution;
  linker/resource suite: 97 subprocess checks per build. These are subordinate
  checks within CTest, not additional CTest test counts.
- ASan heap/global instrumentation and UBSan were enabled. Stack instrumentation
  and leak detection were disabled because of host restrictions; no claim of
  stack/leak sanitizer coverage is made. Parser recursion/resource recovery is
  separately exercised by native C embedding tests.
- Detached and renamed AS1/AS2/AS3/Limbo/FIR/linker projects each built and passed
  their tests using the explicit external generator, without sibling-source
  includes. Detached FIR tested object generation without a linker or VM runner.
- Combined IDE C host linked all four syntax APIs, numeric source compiler, FIR
  optimizer/object compiler and SWL; generated SWF traced `42` and `36`.
- 667 relative documentation links resolved. Windows and Android builds were
  not run; the Termux commands supplied with the patch apply the Git series.

Use each project's CMake/CTest instructions to repeat the checks. Set TMPDIR to
a writable directory where the host has no usable default temporary directory.
FIR integration accepts FIR_SWL and FIR_FLASHVM_RUNNER explicitly; a successful
syntax check alone does not establish backend coverage.
