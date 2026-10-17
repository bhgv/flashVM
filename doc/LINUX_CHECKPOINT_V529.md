# Linux checkpoint v0.529 / Контрольная точка Linux

Recorded by documentation-only v0.530. Implementation revision:
`fc6142217880d950b9cd127c644fde7f091642cf` (v0.529). This checkpoint records
validated work on stages 1/6/7/8/9; it closes none of the nine roadmap stages.

| Linux Release profile | Passing CTest cases | Unicorn instruction/ABI cases | Audited native ELF artifacts |
| --- | ---: | ---: | ---: |
| Minimal | 1095 | 26 | 1084 |
| AGG | 1115 | 26 | 1104 |
| Cairo | 1115 | 26 | 1104 |

AGG/Cairo include the optional reference runner, fonts, codecs, SQLite and native
ports. Their final complete `validate_platform.py --jit-emulator
--elf-dependencies` records report success, v0.529, the revision above and a clean
source tree. Compiler identity: GNU C/C++ 13.3.0, Linux x86-64, 8-byte pointers;
CMake/CTest 4.4.4. C90/C++98 and active Release assertions are retained.

All three direct DT_NEEDED audits found only libc, libgcc_s, libm and libstdc++.
They reject dynamic bundled-library sonames; they do not inspect transitive
libraries or dlopen. Unicorn 2.1.4 executes synthetic helper/branch/ABI cases for
x86/ARM/Thumb/ARM64/RV32/RV64. Native x64 VM behavior is covered by the host suite;
synthetic foreign instructions do not establish a foreign-OS VM/GC run.

Nine targeted ASan/UBSan cases cover typed/recreated/native-reentrant Array length,
GC alias/shared-edge controls, malformed graphs and both allocation sweeps.
Stack instrumentation was enabled; LeakSanitizer was explicitly disabled.
The standalone DEFS/CMake contract checker passed all 26 configuration cases.

The workspace intermittently lost user-execute bits on generated ELF files.
Initial launches were correctly reported as failures, not passes. Minimal was
repaired/rerun as documented in ARRAY_LENGTH_CACHE_GC.md. AGG required restoration
of 21 execute bits and an additional repeat after those permissions reverted;
Cairo required 19. Final complete AGG/Cairo runs passed without source changes.
Generated logs, JSON evidence, copied profiles and fixtures remain outside Git.

Implementation changes in this series are documented in ARRAY_TYPED_LENGTH.md,
ARRAY_LENGTH_REASSIGNMENT.md and ARRAY_LENGTH_CACHE_GC.md. They retain typed
length data independently of conversion, select live descriptors after callbacks,
allow loaded virtual length, and remove obsolete hidden GC edges.

## Русский

v0.530 фиксирует результаты без новых изменений исполнения. Проверена реализация
v0.529 из указанного коммита: minimal — 1095 тестов, AGG/Cairo — по 1115;
дополнительно по 26 сценариев Unicorn и аудит 1084/1104/1104 ELF. Итоговые
полные запуски AGG/Cairo успешны на чистом дереве исходников. Проверены GNU
C/C++ 13.3.0, Linux x86-64, CMake/CTest 4.4.4, C90/C++98 и Release с assertions.

Девять проверок ASan/UBSan прошли с инструментированием стека и отключённым
LeakSanitizer; все 26 проверок конфигураций DEFS/CMake успешны. При потере прав
исполнения начальные прогоны корректно завершались ошибкой. После восстановления
прав и повторных запусков все перечисленные проверки прошли, исходники между
попытками не менялись. Сгенерированные результаты в Git не добавляются.

Это Linux HEADLESS и прямые DT_NEEDED; не проверены transitive/dlopen зависимости,
полная VM/GC на других CPU/ОС и реальные запуски Windows/macOS/Android.
Полная SWF/AS совместимость, остальные функции роудмапа и приёмка всех девяти
стадий остаются открытыми. Количество тестов не означает полноту реализации.
