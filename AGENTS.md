# FlashVM development rules

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
- Keep public APIs C89-compatible and implementation C++98-compatible, without
  STL containers. Keep platform adapters in `ports/` and bundled static library
  sources in `external/`.
- Write readable code with useful comments, verify affected builds and tests,
  and document supported behavior and remaining limitations in Russian and English.
- In C examples, explain purpose, API semantics, ownership and cleanup with
  concise comments. Use readable C89 declarations and the project brace style;
  avoid comments that merely repeat the syntax.

- Never commit generated build/test logs, validation JSON summaries, copied build
  profiles, or source-hash dumps. Keep them outside the repository or in ignored
  build output. Commit actual tests, reusable build profiles and concise reports.

- If validation artifacts need preserving, deliver them in a separate versioned
  ZIP for download. Keep generated test results, copied profiles and logs out of
  both the source tree and committed Git history.
