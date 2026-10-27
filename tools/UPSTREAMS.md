# Owned build sources and update policy

[Русский](UPSTREAMS.ru.md) · [First steps](../README-FIRSTSTEPS.en.md)

Root builds no longer require downloading a ready-made Coco/R or Limbo SDK.
Only selected source files from the owner's repositories are bundled. The
`upstream/` trees are exact committed blobs; first-party CMake adapters, tests
and documentation live outside them. Original notices and licence texts remain.
The workspace MIT licence does not replace upstream component licences.

| Folder | Upstream | Branch / pinned revision | Purpose |
| --- | --- | --- | --- |
| [coco-r-C](coco-r-C/README.md) | https://github.com/bhgv/coco-r-C | master / 9eb70dbf3839d69a0cdcf9e20e3eaafaa87da89d | C generator and C frames actually used by our compiler grammars |
| [coco-r-limbo](coco-r-limbo/README.md) | https://github.com/bhgv/coco-r-limbo | main / f76ae3d40cd00c4829ea1b36f34456898cec98db | C-to-Limbo generator and source Limbo generator, runtime fragments |
| [limbo-bootstrap](limbo-bootstrap/README.md) | https://github.com/bhgv/Inferno-OS-bhgv | experiment / 381ecd2f8162bc02d9bda6cedfde9aad9fb8bbe6 | Temporary native compiler, mature debug/native-stub reference, C and Limbo source versions |
| [../disVM](../disVM/README.md) | https://github.com/bhgv/Inferno-OS-bhgv | experiment / 381ecd2f8162bc02d9bda6cedfde9aad9fb8bbe6 | Interpreter core, 64-bit leaf JITs and selected regressions |

Each folder's `UPSTREAM.json` is authoritative for repository, branch, resolved
commit, selection rules and SHA-256 of every imported file. No `.git` directory,
prebuilt tool, whole OS, sample archive or unrelated documentation tree is copied.
C++ Coco/R frames are excluded. The Limbo source generator's runtime fragments
are required generation inputs. Some module interfaces, including Draw->Context,
are retained for bootstrap signatures; GUI implementations are not linked.

## Verify without network or edits

```sh
python3 tools/vendor-sync.py tools/coco-r-C
python3 tools/vendor-sync.py tools/coco-r-limbo
python3 tools/vendor-sync.py tools/limbo-bootstrap
python3 tools/vendor-sync.py disVM
python3 tools/test_vendor_sync.py
```

## Refresh an active upstream

Keep the upstream checkout outside this repository. Update its selected branch
with your normal Git workflow, then import its committed revision explicitly:

```sh
git -C /absolute/Inferno-OS-bhgv pull --ff-only https://github.com/bhgv/Inferno-OS-bhgv.git experiment
python3 tools/vendor-sync.py disVM --update --source /absolute/Inferno-OS-bhgv --ref experiment
python3 tools/vendor-sync.py tools/limbo-bootstrap --update --source /absolute/Inferno-OS-bhgv --ref experiment
./build-workspace.sh --projects disvm,limbo-bootstrap --tests
```

The helper never fetches, checks out, commits or pushes. It reads `git show` from
one resolved commit, ignoring dirty checkout files. It refuses symlinked paths,
changed/missing recorded files and collisions with unrecorded local files.
Only manifest-owned obsolete files can be removed. New blobs are fully staged
before publication, with the manifest published last. An interrupted multi-file
update is detected by verification; it is not claimed directory-atomic.
Review the Git diff and rerun affected builds. A new upstream include/dependency
may require an explicit whitelist/CMake update; this is deliberately not a full
repository mirror. Do not silently import new OS/GUI/security dependencies.

## Fixing upstream defects

Confirmed defects belong in the owning repository, not as hidden vendored edits.
Make a local work branch there, fix and test the original C or Limbo source,
commit with the user's identity and export a separate format-patch. Refresh this
workspace from that exact local commit. Give the owner both upstream and FlashVM
patches when both repositories change. Publication remains manual; no automatic
push is part of this workflow. This import required host/build adapters, not a
confirmed upstream defect fix; upstream blobs remain unchanged.

## Primary compiler priority

The legacy native Limbo compiler is a bootstrap tool, not our completed compiler.
Its historical yacc-generated parser is kept as a bootstrap source artifact and
requires no yacc installation to build. Our AS/Limbo/FIR parsers remain Coco/R.
Priorities for our compiler are source/debug information, semantic lowering,
Dis/SWF emission and `.m`-driven C/AS/other-language external module support.
See [the own compiler contract](../compiler/limbo/OWN_COMPILER.md).

[Inferno mk](mk/README.md) · [Inferno iyacc](iyacc/README.md): selected experiment f948343b711c8300dbf76417254c18a7a8d2f5ff, 82 / 17 upstream files, explicit dependencies and bootstrap scripts.
