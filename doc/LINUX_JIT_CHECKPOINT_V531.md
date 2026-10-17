# Linux automatic-JIT checkpoint / контрольная точка Linux JIT

This report records implementation **v0.531**, commit
`09005a651cf84385bc18bf9dab46401ad7f16172`. v0.532 adds this report and updates
the development version; it changes no execution policy or native instructions.
All nine completion-roadmap stages remain open.

## Evidence

Linux x86-64, GNU C/C++ 13.3.0, CMake/CTest 4.4.4, Python 3.12.14,
Unicorn 2.1.4. Release builds require C90/C++98, enable assertions and use the
bundled static dependencies selected by each profile.

| Scope | Result |
| --- | --- |
| minimal, precommit implementation validation | 1096 CTest cases passed |
| minimal-jit, precommit implementation validation | 1096 CTest cases passed |
| minimal-jit, clean committed release-runner build with reference runner | 1097 CTest cases passed |
| Fresh synthetic ISA/ABI fixtures for the clean build | 26 Unicorn cases passed |
| Direct ELF dependency audit of the clean build | 1086 artifacts passed |
| Definition acceptance/rejection and cache conflict configurations | 30 cases passed |
| Platform encoder/evidence unit suite | 10 cases passed |
| Targeted Debug ASan/UBSan | 3 cases passed |

The clean runner recorded the exact commit above, version 0.531, `dirty=false`
and overall/profile/gate status passed. It configured and built its own separate
directory, discovered tests through CTest and generated new encoder fixtures.
The reference runner accounts for the additional release-runner CTest case.
The ELF audit found only libc.so.6, libgcc_s.so.1, libm.so.6 and libstdc++.so.6
in direct DT_NEEDED entries; this is not a transitive dependency/dlopen audit.

The ASan/UBSan cases were bitmap_load_resource, execution_build_default and
script_native_function2, with stack-use-after-return enabled and LeakSanitizer
disabled. These targeted cases do not certify all memory paths.

The first automatic-JIT suite exposed the root native receiver metadata bug
fixed in v0.531: BitmapData.loadBitmap selected global _root instead of the
calling MovieClip. The resource test now compares interpreter/manual/automatic
JIT, GC and graph transfer. The native/interpreter comparison test also explicitly
selects its intended manual mode rather than relying on a build default.

Initial native runs also encountered lost execute bits on generated ELF files.
After repair, final full suites passed. The clean release runner initially failed
closed on 16 such launch failures; after restoring their execute bits, the whole
runner was repeated and all gates passed.

## Reproduction and limits

With the named tools installed, run from the checkout:

```sh
python3 scripts/validate_platform.py --profile minimal-jit --jobs 6 \
  --jit-emulator --elf-dependencies
```

Use --cmake/--ctest for non-PATH installations. The new profile starts contexts
in automatic JIT mode, with existing interpreter fallback and runtime overrides;
see [the contract](JIT_BUILD_DEFAULT.md). The default DEFS remains INTERPRETER.
This checkpoint does not rerun AGG/Cairo at v0.531; their earlier v0.529 evidence
is recorded separately in [LINUX_CHECKPOINT_V529.md](LINUX_CHECKPOINT_V529.md).

Native VM evidence here is Linux x86-64. Unicorn executes generated x86, ARM,
Thumb, ARM64, RV32 and RV64 instruction/ABI fixtures with synthetic helpers;
it does not run the complete VM, collector or foreign OS. Real Windows/macOS/
Android builds and runs, full foreign CPU acceptance, full AVM1/SWF/media
compatibility and roadmap release acceptance remain open.

## Русский

Проверена реализация v0.531 на указанном коммите. v0.532 содержит только эту
контрольную точку и новую версию документации. На Linux x86-64 прошли по 1096
тестов двух профилей; отдельный чистый прогон minimal-jit с reference runner
прошёл 1097 тестов, 26 свежих Unicorn-проверок и аудит 1086 ELF. Отчёт зафиксировал
точный коммит, dirty=false и успешность всех фаз. Дополнительно прошли 30
проверок конфигурации, 10 тестов инфраструктуры и три целевых ASan/UBSan-теста.

Исправлена потеря метаданных receiver native-кадра, из-за которой loadBitmap
выбирал глобальный _root. Ресурсный сценарий проверен в трёх режимах, с GC и
переносом графа. После восстановления потерянных execute-битов сгенерированных
ELF весь release runner запущен заново и завершился успешно.

LeakSanitizer отключён; ELF-аудит проверяет только прямые зависимости. Unicorn
не заменяет запуск всей VM на других CPU/ОС. AGG/Cairo в этой контрольной точке
заново не проверялись. Реальные Windows/macOS/Android, полная совместимость SWF,
мультимедиа и приёмка всех девяти этапов остаются открытыми.
