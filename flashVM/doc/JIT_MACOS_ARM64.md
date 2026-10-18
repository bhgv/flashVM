# Darwin ARM64 integer-call encoder / Генератор Darwin ARM64

## Scope / Область реализации

v0.568 adds the private offline target [`FVM_JIT_TARGET_MACOS_ARM64`](../src/jit/backend.hpp#L18). It lowers
the existing decoded helper-call plan; it does not directly compile arbitrary
C/C++, Swift, floating-point/variadic calls or arm64e authenticated pointers.
There is no new public C ABI. v0.568 was offline-only; the v0.569 runtime
publication section below describes the separate opt-in and its native acceptance limits.

Добавлен отдельный offline target: он генерирует машинные инструкции для уже
подготовленного плана вызовов JIT. Наличие байтов не означает разрешения ОС на
их исполнение. В v0.568 реализована только генерация инструкций. Раздел v0.569 ниже описывает
добавленный включаемый MAP_JIT-переходник; нативная приёмка остаётся открытой.

## Calling convention / Соглашение вызовов

Apple's [ARM64 ABI documentation](https://developer.apple.com/documentation/xcode/writing-arm64-code-for-apple-platforms)
reserves X18 and requires a valid frame record through X29. The separate
[`fvm_jit_machine_macos_arm64`](../src/jit/machine.hpp#L32) prologue preserves X19/X20, saves X29/LR at SP+16
inside a 32-byte aligned frame and links X29 to that record before calling a
helper. The epilogue restores the entire frame on normal and error exits.
X18 is never used. The emitter neither consumes nor stores red-zone data.

X19 retains the invocation pointer across helper calls. Each shared
[`fvm_jit_machine_arm64`](../src/jit/arm64.cpp#L17) call sequence reinstalls X0 and a zero-extended unsigned
32-bit operand in W1, loads the full helper address into X16, then executes BLR.
The shared [`fvm_jit_encode`](../src/jit/backend.hpp#L30) relocation path patches AArch64 B instructions,
including backwards branches and conditional branches beyond the CBZ range.
These fixed two-argument helpers do not require stack arguments or variadic rules.
No unwind-table registration or pointer authentication is provided.

X19 хранит полный указатель invocation; X0 и W1 восстанавливаются перед каждым
вызовом. X16 содержит полный адрес функции, X18 не используется. Новый пролог
сохраняет X19/X20 и связанную пару X29/LR; стек остаётся выровненным на 16 байт.
Общий генератор вызовов используется повторно только для совместимой части ABI,
а не для Linux-пролога, в котором нет Darwin frame chain. Ошибка обработчика
переходит в общий эпилог с восстановлением сохранённых регистров и стека.

## Reproduce / Воспроизведение

Build the minimal automatic-JIT profile and emit fresh synthetic-address fixtures:

```sh
cmake -S . -B build-jit -DFVM_DEFS_FILE="$PWD/cmake/profiles/minimal-jit.defs" -DCMAKE_BUILD_TYPE=Release
cmake --build build-jit --parallel 4
mkdir -p build-jit/encoder-fixtures
build-jit/flashvm_tests_jit_backends build-jit/encoder-fixtures
python3 -m pip install unicorn
python3 scripts/check_jit_backends.py build-jit/encoder-fixtures
ctest --test-dir build-jit --output-on-failure
```

The emulator checker is optional, requires Python 3 with assertions enabled and
Unicorn, and executes actual emitted instructions. The CTest generator itself
checks output generation, pointer widths and rejected branch destinations; it is
not equivalent to emulator execution. Keep fixtures/logs in ignored build output.

Проверки Unicorn: прямые/обратные переходы, ранний ошибочный возврат, дальние
переходы через тело более 1 МиБ, старший бит unsigned operand, адреса функций
и invocation выше 4 ГБ, выравнивание стека, сохранение X19/X20/X29 и X18, реальные
связанные frame records и восстановление caller LR. Обработчики намеренно портят
регистры аргументов. Неверная цель перехода оставляет выходной буфер без изменений.
Пять прежних offline-архитектур проверяются тем же запуском.

These are CPU-emulator checks on Linux, not an Apple SDK build, native Apple
Silicon run, OS crash-backtrace test or full AVM1 equivalence evidence. All nine
roadmap stages remain open. The v0.568 checkpoint required a separate macOS ARM64 memory/signing policy;
v0.569 supplies an opt-in port below, with native verification still outstanding.

## Sources / Исходники

- Target and width: [backend.hpp](../src/jit/backend.hpp), [backend.cpp](../src/jit/backend.cpp).
- Darwin frame: [macos_arm64.cpp](../src/jit/macos_arm64.cpp), [frame constants](../src/jit/macos_arm64_constants.h).
- Shared calls/relocations: [arm64.cpp](../src/jit/arm64.cpp), [encode.cpp](../src/jit/encode.cpp).
- Fixtures: [jit_backends.cpp](../tests/jit_backends.cpp), [emulator checker](../scripts/check_jit_backends.py).
## v0.569: opt-in macOS 11.4+ publication / включаемая публикация

The v0.568 section above records the offline checkpoint. v0.569 adds an opt-in
runtime port. The default remains OFF while native Apple SDK/OS acceptance is
outstanding. Desktop ARM64, LP64 and a deployment target of at least macOS 11.4
are required. arm64e, iOS and older deployment targets remain excluded.

```sh
cmake -S . -B build-macos-arm64 \
  -DFVM_DEFS_FILE="$PWD/cmake/profiles/minimal-jit.defs" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.4 -DFVM_ENABLE_MACOS_ARM64_JIT=ON
cmake --build build-macos-arm64 --parallel 4
codesign --force --sign - --options runtime \
  --entitlements examples/embedding/jit/macos-arm64.entitlements \
  build-macos-arm64/flashvm_examples_embedding_jit_main
build-macos-arm64/flashvm_examples_embedding_jit_main
ctest --test-dir build-macos-arm64 --output-on-failure
```

These are native-host preparation commands, not commands executed on an Apple
machine for this checkpoint. Sign other test/application executables separately
when Hardened Runtime policy requires it. The library never changes signatures,
entitlements or the host's callback-freeze policy. Statically link the library
into the executable present at process startup; late dlopen callback allowlisting
needs a separately coordinated host policy and is not provided here.

Команды предназначены для настоящего macOS ARM64 хоста; здесь они не выдаются за
выполненную Apple-приёмку. Подпись теста не подписывает остальные исполняемые
файлы. Пример entitlement относится к ARM64-переходнику, а Intel-файл остаётся
отдельным. Без opt-in сборка не выбирает ARM64 JIT; при отказе mmap явная компиляция
возвращает ошибку, автоматический режим может использовать interpreter fallback.

### One region, many bodies / одна область и много тел

Apple's [JIT porting guide](https://developer.apple.com/documentation/apple-silicon/porting-just-in-time-compilers-to-apple-silicon)
limits a Hardened Runtime application with allow-jit to one MAP_JIT region.
[`fvm_mac_arm64_code_create`](../ports/mac/code_arm64.cpp#L159) therefore lazily reserves one process-lifetime
[`FVM_MAC_JIT_ARENA_BYTES`](../ports/mac/code_arm64_constants.h#L5) region (64 MiB), then assigns aligned non-overlapping
slots of [`FVM_MAC_JIT_SLOT_ALIGNMENT`](../ports/mac/code_arm64_constants.h#L7) bytes (16-byte alignment). Out-of-band
metadata and a pthread mutex protect allocation, splitting, coalescing and
callback publication. Context teardown returns its slots, not the mapping.
The virtual reservation remains until process exit; no library shutdown/remap
protocol or cross-library MAP_JIT pool negotiation is provided. Another library
may already own the OS-permitted region; mmap can then fail.

[`fvm_mac_arm64_code_release`](../ports/mac/code_arm64.cpp#L182) returns an inactive slot and merges neighbouring free
slots. Other live bodies retain their addresses and bytes. A fragmented/full
pool returns [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44). The shared reservation, slot padding and native
metadata are not counted as per-context retained instruction bytes; the existing
JIT source/code budgets remain separate. No managed objects or GC roots live in
the arena metadata. Caller ownership must exclude release while any thread could
execute the body; raw stale pointer/cookie pairs do not acquire generation safety.

Один процессный пул обслуживает несколько контекстов и вложенных тел. Освобождение
ссылки на код возвращает участок и объединяет соседние свободные участки, не
сдвигая действующий код. Регион остаётся до завершения процесса; учёт лимитов
контекста не равен полному процессному резерву 64 МиБ. Пул не координирует владение
MAP_JIT с другими JIT-библиотеками. Уже занятый ими системный регион может привести
к UNSUPPORTED; переполнение/фрагментация собственного пула — к OUT_OF_MEMORY.

### Write protection / защита записи

The [Apple pthread header](https://github.com/apple-oss-distributions/libpthread/blob/main/include/pthread/pthread.h)
declares pthread_jit_write_with_callback_np for macOS 11.4+. The C++98 SDK boundary
uses PTHREAD_JIT_WRITE_ALLOW_CALLBACKS_NP to register exactly one copy callback.
The adapter never uses pthread_jit_write_protect_np, which is incompatible with
the jit-write-allowlist entitlement. It rejects a missing per-thread protection
facility rather than relying on unprotected RWX. The MAP_JIT mapping request
contains READ/WRITE/EXEC permissions, while the OS callback enforces thread-local
writable-or-executable access; this differs from the Intel mprotect RW-to-RX path.

The callback validates its active synchronous request and reserved slot, copies
only instruction bytes, and does not allocate, invoke user code or perform a
non-local transfer. Only after the system restores executable protection does
the adapter invalidate the instruction cache and publish caller outputs.
The host must enter from normal executable mode, not recursively from another
JIT write callback. Coordination with independently changing per-thread JIT
protection is the host's responsibility. This is not an isolation boundary against
an application that already allows arbitrary writes into native process memory.

Запись выполняется только через зарегистрированный callback: после возврата ОС
восстанавливает исполнимую защиту потока, затем синхронизируется instruction cache
и публикуется указатель. Внутри callback нет выделения памяти, пользовательского
кода, longjmp или исключений. Метаданные находятся вне JIT-региона. Host не должен
вызывать компиляцию рекурсивно из чужого write callback или оставлять текущий
поток в несовместимом режиме записи. Другие потоки могут выполнять прежний код,
пока выделяется и заполняется отдельный не опубликованный участок.

### Private port contracts / внутренние контракты

- `fvm_mac_arm64_code_supported()` takes no arguments and reports the compiled
  policy plus OS per-thread protection support. It does not prove signing permission.
- `fvm_mac_arm64_code_create(bytes, size, out, cookie)` requires readable nonempty
  compiler-owned bytes and non-NULL outputs. Success copies bytes and returns a
  live slot plus its aligned size as an opaque release cookie. Failures leave
  both outputs unchanged: INVALID_ARGUMENT for invalid inputs, UNSUPPORTED for
  disabled/unavailable policy, oversize bodies, mapping policy or callback failure,
  OUT_OF_MEMORY for metadata/mapping allocation or pool exhaustion, ERROR if the
  arena mutex cannot be acquired. It never enters ActionScript or establishes GC roots.
- `fvm_mac_arm64_code_release(memory, cookie)` returns void; NULL, zero and
  mismatched live pointer/cookie pairs are ignored. Release only after all native
  users finish. Do not copy/reuse stale ownership tokens. The shared mapping stays
  alive independently of VM lifetime. Synchronization protects the metadata,
  not the caller's function/code handles.

Внутренние функции не меняют публичный C ABI. Успех create передаёт один участок
вызывающему; каждый такой участок требует одного release после завершения всех
исполнений. При ошибке результаты не изменяются. Поддержка SDK не удостоверяет
разрешение подписи: отказ защиты/mmap остаётся проверяемым результатом компиляции.

### Validation boundary / границы проверки

Controlled SDK tests compile and execute the actual arena implementation on Linux.
They verify allocation/mmap/callback failure rollback, unchanged outputs, one
mapping for multiple bodies, exact-capacity exhaustion, splitting/coalescing,
wrong-cookie rejection, cache-after-protection ordering and 800 allocations/releases
across four real pthread workers. These test copies are ordinary allocated memory:
they do not establish real Apple W^X or MAP_JIT enforcement. The actual selector and
platform header are tested separately across opt-in, deployment and ABI exclusions.
The native macOS fixture explicitly skips on a non-Apple host or unavailable policy.
Unicorn instruction/frame evidence remains separate from SDK protocol evidence.

Проверки с подменённой SDK-границей не являются запуском на Apple Silicon, проверкой
Mach-O callback allowlist или доказательством поведения codesign. Нативные сборка,
подпись, исполнение нескольких VM/потоков и crash unwind на macOS ещё необходимы.
Все девять этапов роудмапа остаются открытыми.
## v0.570: native acceptance scenarios / нативные сценарии приёмки

`flashvm_tests_jit_native_instances` runs on any available native backend. It keeps
two separately compiled bodies and two runtime/context owners alive, checks actual
native-entry counters in MANUAL mode, destroys the first owner, calls the survivor,
checks its original source bytes, compiles a replacement and calls it before final
cleanup. On macOS, `flashvm_tests_jit_macos_native` delegates the same contract after
checking the selected Darwin backend name. This avoids counting a correct number
returned by interpreter fallback as native execution.

`flashvm_tests_jit_macos_arm64_native` is Apple Silicon only. Its helpers inspect
the linked frame record, check return-address provenance and exercise preserved
X19/X20, unsigned operands, branches and error returns through actual published
code. It keeps two bodies live, releases one, publishes another and calls both
survivors. The target uses frame pointers explicitly. It never modifies or relies
on the OS-owned X18 value. Sign this executable using the ARM64 entitlement file
before running under the host's Hardened Runtime policy.

```sh
codesign --force --sign - --options runtime   --entitlements examples/embedding/jit/macos-arm64.entitlements   build-macos-arm64/flashvm_tests_jit_macos_arm64_native
ctest --test-dir build-macos-arm64 --output-on-failure   -R 'jit_(macos_arm64_native|macos_native|native_instances)$'
```

Sign the other two executables separately when required. A first publication
rejected with UNSUPPORTED, a disabled backend or a foreign host reports explicit
skip 77. Once the first publication succeeds, failure to create the simultaneous
second body is a test failure, not a policy skip. The two runtime instances are
interleaved on one owning thread; this does not certify concurrent use of one VM.

В Linux выполнен общий сценарий с настоящим JIT и его ASAN/UBSAN-прогон, без
LeakSanitizer. macOS-сценарии на Linux дают явный skip. ARM64-тело проверено на
C++98-синтаксис с контролируемыми platform macros; это не компиляция Apple SDK,
не проверка ассемблера Apple Clang и не нативное исполнение. Нативные сборка,
подпись и запуск этих сценариев остаются обязательными перед закрытием этапа 9.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_jit_encode` | function | [src/jit/backend.hpp:30](../src/jit/backend.hpp#L30) |
| `fvm_jit_machine_arm64` | function | [src/jit/arm64.cpp:17](../src/jit/arm64.cpp#L17) |
| `fvm_jit_machine_macos_arm64` | function | [src/jit/machine.hpp:32](../src/jit/machine.hpp#L32) |
| `FVM_JIT_TARGET_MACOS_ARM64` | enum constant | [src/jit/backend.hpp:18](../src/jit/backend.hpp#L18) |
| `fvm_mac_arm64_code_create` | function | [ports/mac/code_arm64.cpp:159](../ports/mac/code_arm64.cpp#L159) |
| `fvm_mac_arm64_code_release` | function | [ports/mac/code_arm64.cpp:182](../ports/mac/code_arm64.cpp#L182) |
| `fvm_mac_arm64_code_supported` | function | [ports/mac/code_arm64.cpp:149](../ports/mac/code_arm64.cpp#L149) |
| `FVM_MAC_JIT_ARENA_BYTES` | macro / define | [ports/mac/code_arm64_constants.h:5](../ports/mac/code_arm64_constants.h#L5) |
| `FVM_MAC_JIT_SLOT_ALIGNMENT` | macro / define | [ports/mac/code_arm64_constants.h:7](../ports/mac/code_arm64_constants.h#L7) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
<!-- source-links:end -->
