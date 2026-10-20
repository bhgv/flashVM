# Native release evidence

v0.642. [Русский](RELEASE_EVIDENCE.ru.md). Roadmap stage 9 remains open.

[validate_platform.py](../scripts/validate_platform.py) now writes schema 2
platform-evidence.json. Each profile retains commands, compiler identities,
source endpoints and exact CTest execution evidence. Schema 1 historical reports
remain historical evidence and are not upgraded by relabeling their schema.

[inventory](../scripts/ctest_evidence.py) reads CTest JSON after the build;
[results](../scripts/ctest_evidence.py) requires one JUnit case for every configured
name, without duplicates, failures, disabled tests or unknown statuses. A second
inventory must match after execution. Old XML is removed before testing. Missing,
malformed, oversized or incomplete output is a failed run even if CTest exited
zero. These are local provenance checks, not cryptographic attestation.

Skipped tests require repeated `--allow-skip EXACT_TEST_NAME` options. There are
no wildcard patterns. Every actual skip is retained by name, and never counted
as passed; zero executed successful cases is rejected. This intentionally makes
default validation stricter than earlier releases. Inspect the native platform's
actual skips before explicitly acknowledging an unsupported platform protocol.
An acknowledgement is a documented limitation, not proof of that capability.

From the component directory:

```sh
python3 scripts/validate_platform.py --profile minimal --jobs 2
```

Use explicit `--cmake` and `--ctest` paths if the tools are outside PATH. The
complete run builds all configured targets; there is no acceptance subset filter.
Generated logs, JUnit and JSON remain in ignored build-platform directories.
Changing source inputs or test definitions during a run invalidates its result.
Do not commit generated validation results. The source tree and compiler/library
dependencies still need their separate static-link and native-platform checks.

Regression evidence: nine CTest-evidence tests include actual filtered, skipped
and disabled CTest runs; seven source-identity tests verify source drift and
failure replacing an earlier successful report. These are Linux tool tests,
not native Windows/macOS/Android acceptance or full Flash conformance.

## Native target and ABI — v0.642

[native_target.py](../scripts/native_target.py) reads only the active CMake
version directory; stale CMakeSystem.cmake files cannot replace or invalidate it.
Literal complete nonconflicting host/target fields and FALSE cross mode are
required. OS, normalized architecture and both compiler pointer widths must
match the running reporter. An Android or ARM target on Linux x64 is rejected
before build/test. A 32-bit target from 64-bit Python requires a separate matching
validation environment. The native reporter covers Linux/Windows/macOS; Android
requires separate device acceptance.

Seven tests validate metadata policy; Windows/macOS/RISC-V cases are fixtures,
not foreign-platform executions. The actual active Linux x86-64 GNU C/C++
configuration separately passed the identity check.
