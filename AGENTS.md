# FlashVM development rules

- Start every new commit subject with `v<version> `, using the development
  version in `VERSION.txt`, followed by a meaningful English description.
  Example: `v0.1 Fix native constructor root lifetime`.
- Change the version for an intentional development milestone; a version number
  does not certify full SWF or platform compatibility. Keep ABI versions separate.
- The internal Git repository and delivered archive are authoritative. Update
  the remote mirror only on an explicit user command. Do not publish private
  stashes or rewrite existing history without explicit authorization.
- Keep public APIs C89-compatible and implementation C++98-compatible, without
  STL containers. Keep platform adapters in `ports/` and bundled static library
  sources in `external/`.
- Write readable code with useful comments, verify affected builds and tests,
  and document supported behavior and remaining limitations in Russian and English.
