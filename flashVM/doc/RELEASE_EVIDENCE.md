# Native release evidence

v0.645. [Русский](RELEASE_EVIDENCE.ru.md). Roadmap stage 9 remains open.

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

## Aggregate matrix — v0.643

[release_matrix.py](../scripts/release_matrix.py) consumes portable report
folders: platform-evidence.json and profile subdirectories containing
test-inventory.log, test-inventory-after.log and ctest-results.xml. All four
files are required; JSON alone is insufficient. Recorded absolute foreign-host
paths are never executed. Artifacts, including symlinks, must stay in the bundle.

Require one version/full commit ID, a clean worktree, matching source endpoints
within each report, valid ABI and command ordering. Cross-OS source hashes need
not match because filesystem modes differ. Inventory/JUnit hashes and counts
are recomputed. Duplicate cells, modified artifacts, schema 1, dirty sources or
another commit fail. Skips need independent exact OS/architecture/profile/name
acknowledgements.

```sh
python3 scripts/release_matrix.py --commit FULL_COMMIT_ID --version 0.643 \
  --report /evidence/linux/platform-evidence.json \
  --report /evidence/windows/platform-evidence.json \
  --output /results/release-matrix.json
```

The default matrix has 16 cells: agg/cairo/minimal/minimal-jit for Linux x86_64,
Windows x86_64, macOS aarch64 and Android aarch64. The desktop reporter cannot
supply Android evidence; device acceptance remains open. Missing cells return
1/blocked; invalid evidence returns 2/invalid. Explicit
`--require Linux:x86_64:minimal` checks a subset only; its exit 0 does not close
stage 9. Output includes exact cells, limitations and scope.
`--allow-skip Linux:x86_64:minimal:EXACT_NAME` acknowledges one skip. Output must
be outside every input bundle; JSON is published atomically. Reports are not
signed: deliberate coordinated fabrication is outside this local tool's scope;
it does not replace trusted CI/native execution.

Eleven tests cover missing platforms, relocation, wrong ABI/version/commit,
skips, altered JUnit, duplicates, escaping symlinks and replacing previous success
with blocked/invalid output. These fixtures are not foreign-platform runs.

## Portable evidence archive — v0.644

[release_bundle.py](../scripts/release_bundle.py) exports only the report,
two CTest inventories and JUnit per profile, plus manifest.json with sizes/SHA-256.
No binaries, arbitrary logs, sources or Git history are included. A staged
snapshot is validated before writing, so edits between initial validation and
copying cannot publish inconsistent evidence. Fixed member order, 1980 timestamps,
0644 permissions and ZIP_STORED make repeated exports of identical input bytes
identical. Independent native runs naturally differ in times and recorded paths.

```sh
python3 scripts/release_bundle.py --commit FULL_COMMIT_ID --version 0.644 \
  --report /evidence/linux/platform-evidence.json \
  --output /exports/native-linux-v0.644.zip
```

Actual skips require the same exact --allow-skip rules as matrix evaluation.
Output must be outside the input folder. Publication is atomic; a failed export
preserves any old archive, so checking exit status is mandatory. Extract your
trusted producer archive into a separate folder and pass platform-evidence.json
to the matrix tool. Manifest hashes provide integrity, not signatures/OS attestation.

Five tests cover byte determinism, manifest contents, relocation, omission of
unrelated files, concurrent edits and old-archive preservation after errors.
A separate actual CMake/C89/C++98/CTest miniature pipeline demonstrates rejection
of zero-exit CTest with an unacknowledged skip, successful retry, matrix audit,
export and recovery after another failure. It tests tooling with a small fixture,
not the complete VM suite.

## Timeout and encoding — v0.645

`validate_platform.py --test-timeout 300` sets the CTest default timeout for
tests without an explicit TIMEOUT property; positive seconds are required.
This is not a total build/run deadline and does not replace explicit timeouts
on endurance tests. The value is recorded per profile. A real sleeping child
was terminated by CTest at a one-second default, leaving a failed report.
JUnit is UTF-8 only: UTF-16/32 cannot hide forbidden DTD/entity declarations
behind zero bytes. An alternate-encoding test extends the original nine checks.

This series changes acceptance infrastructure, not VM semantics. It does not
claim a complete run of every registered VM test, new ASan/UBSan coverage,
physical devices, Windows/macOS/Android runs or closure of any roadmap stage.
Use explicit CMake/CTest paths outside PATH and a writable TMPDIR where needed.
Keep generated results/logs outside commits.

## Final v0.641–0.645 series validation

Linux minimal Release: the entire core built in C89/C++98; three selected runtime
checks (AVM1 number text, execution conversions, JIT numbers) passed against the
final core. All 15 selected CTests passed: three runtime, ten tooling and two
documentation/tutorial contracts. Five new CTest groups contain 34 checks,
including a real native miniature project and actual timeout. The remainder of
the VM suite was not rerun. All 812 relative links in modified documents resolved,
and generated source links passed freshness checks. An initial parallel build
encountered an assembler CFI error; retry with two jobs and final target rebuilds
succeeded. This was not a VM runtime failure.
