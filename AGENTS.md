# FlashVM development rules

- Standing user instruction on 2026-10-17: commit directly on `master`.

- Read `doc/ROADMAP.md` before continuing development. It is the user-approved
  completion plan and the primary reference for scope, sequence and acceptance.
  Keep compatibility status and test evidence current; do not mark a stage
  complete without the tests and documentation required by that roadmap.

- Start every new commit subject with `v<version> `, using the development
  version in `VERSION.txt`, followed by a meaningful English description.
  Example: `v0.1 Fix native constructor root lifetime`.
- Write commit messages in English using up to four lines that explain the
  implementation changes. Describe what changed; do not use these lines for tests.
- Before every new commit, increment the subversion in `VERSION.txt` and update
  the current version in documentation. Use that new version in the commit
  prefix. Append `-v<version>` immediately before `.zip` in delivered Git archive
  names. A version number does not certify full SWF/platform compatibility.
  Keep ABI versions separate. Never rewrite historical commit subjects.
- The internal Git repository and delivered archive are authoritative. Update
  the remote mirror only on an explicit user command. Do not publish private
  stashes or rewrite existing history without explicit authorization.
- Configure `origin` as `https://bhgv.pythonanywhere.com/git/FlashVM/`.
  For authorized mirror updates use
  `git -c http.postBuffer=67108864 push origin HEAD:master`, without force.
  Afterwards fetch `origin master` and verify that the local HEAD is an ancestor
  of `FETCH_HEAD`; report the verified server commit, not just the push exit code.
  Standing user authorization on 2026-10-12: update the mirror after every new
  commit until the user gives another instruction. This supersedes the earlier
  per-update restriction while active; the internal repository stays primary.
- Standing user instruction on 2026-10-15: continue in the existing checkout,
  update only the remote mirror, and do not create/export source or local-Git
  archives unless the user explicitly asks again. This supersedes routine archive delivery.
- Recovery authorization on 2026-10-15: the user permits cloning Git again after
  workspace maintenance removed the checkout. Mirror updates remain authorized;
  source/local-Git archive delivery remains disabled unless explicitly requested.
- User reconfirmed sending to the mirror on 2026-10-15 with "отправь на зеркало"
  after automatic review blocked the v0.366 upload. The destination remains
  https://bhgv.pythonanywhere.com/git/FlashVM/; the standing non-force mirror-only
  instructions above remain active.
- Keep public APIs C89-compatible and implementation C++98-compatible, without
  STL containers. Keep platform adapters in `ports/` and bundled static library
  sources in `external/`.
- Use documented uppercase semantic macros in scoped .h headers for protocol/action IDs,
  resource identities, flags, numeric masks and execution/representation bounds.
  Apply this rule to new code and to meaningful numeric constants in code being edited.
  Keep distinct meanings in separate macro namespaces even when values happen to match;
  ordinary counter initialization and array traversal arithmetic need no artificial aliases.
  Preserve third-party source and independent raw-byte/oracle fixtures.
- Write readable code with useful comments, verify affected builds and tests,
  and document supported behavior and remaining limitations in Russian and English.
- Format and document all first-party source files for human readers. Describe
  each file at its start and each function before its declaration/definition.
  Explain header constants, macros, aggregate types and every data member.
  Group initialization, computation, output and cleanup into readable sections;
  preserve required dependency/error-handling order.
- In examples/, explain every nonempty C/C++/ActionScript/header code line as a
  tutorial, including control-flow boundaries and API ownership.
- Start comments explaining garbage-collector code and roots with `[GC:]` so
  they can be found quickly. Cover marking, finalization, root scopes, native
  call barriers, collector state and allocation accounting.
- Exclude external/, tests/ and build scripts from formatting/commenting passes.
  Preserve upstream library source and keep executable C89/C++98 behavior intact.

- Never commit generated build/test logs, validation JSON summaries, copied build
  profiles, or source-hash dumps. Keep them outside the repository or in ignored
  build output. Commit actual tests, reusable build profiles and concise reports.

- If validation artifacts need preserving, deliver them in a separate versioned
  ZIP for download. Keep generated test results, copied profiles and logs out of
  both the source tree and committed Git history.


## Standing documentation and example requirements (2026-10-17)

- User instruction: keep documentation linked to the source and keep examples
  documented and indexed for every subsequent change, not only this checkpoint.
- In user-facing documentation, link described functions, callback types,
  structs/classes/enums, macros, defines and named constants to their real
  declaration/definition in first-party source. Prefer the public declaration
  for C ABI functions and the complete definition for aggregate types. Include
  the current source line where supported; never guess locations or resolve an
  ambiguous local name to an unrelated symbol. Keep existing manual/TOC links.
- Use [scripts/link_documentation_sources.py](scripts/link_documentation_sources.py)
  after changing documented source or Markdown, then run its `--check` mode.
  It refreshes source links and per-page source tables without editing fenced
  code or section headings. Review links for newly introduced/renamed symbols;
  unrecognized or historical symbols require an explicit explanation, not a
  fabricated link. Recheck relative files, Markdown anchors and source lines.
- Document each public API function in detail: purpose, arguments, return/error
  statuses, ownership/lifetime, execution/thread/GC restrictions and meaningful
  limitations. Link the exact API declaration and relevant implementation where
  useful. Keep Russian and English contracts consistent with actual behavior.
- Every example needs a Markdown README: purpose and use cases, requirements,
  implementation details, ownership/cleanup, build/run commands, expected
  results and limitations. List demonstrated C or script APIs with comments
  naming the guide file and links to relevant guide sections and source
  declarations. Mark sketches/conceptual notes explicitly; do not present them
  as executable examples or compatibility acceptance evidence.
- For new/edited example code, retain human formatting and explain every
  nonempty source/header/ActionScript line. Use ordinary error handling rather
  than assertions in tutorials. Keep helper ownership and side effects clear.
- Maintain [examples/README.md](examples/README.md) when adding, removing or
  changing an example: folder groups, logical thematic subgroups, alphabetical
  subgroup/example order, structured linked contents, detailed descriptions and
  a final flat alphabetical glossary linking every example to its description.
  Keep per-example README/source links current. Companion headers/AS sources
  belong to their example; identify documentation-only entries separately.
- Verify catalog completeness/sorting, API coverage, documented build target
  names/feature prerequisites and links. Run the existing debugger tutorial
  checker when relevant. For documentation-only changes, validate documentation
  and affected version metadata; do not claim unrelated runtime retesting.

Workflow details / RU+EN policy: [doc/DOCUMENTATION_GUIDELINES.md](doc/DOCUMENTATION_GUIDELINES.md).
