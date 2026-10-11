# FlashVM development rules

- Start every new commit subject with `v<version> `, using the development
  version in `VERSION.txt`, followed by a meaningful English description.
  Example: `v0.1 Fix native constructor root lifetime`.
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
- Keep public APIs C89-compatible and implementation C++98-compatible, without
  STL containers. Keep platform adapters in `ports/` and bundled static library
  sources in `external/`.
- Write readable code with useful comments, verify affected builds and tests,
  and document supported behavior and remaining limitations in Russian and English.

- Never commit generated build/test logs, validation JSON summaries, copied build
  profiles, or source-hash dumps. Keep them outside the repository or in ignored
  build output. Commit actual tests, reusable build profiles and concise reports.

- If validation artifacts need preserving, deliver them in a separate versioned
  ZIP for download. Keep generated test results, copied profiles and logs out of
  both the source tree and committed Git history.
