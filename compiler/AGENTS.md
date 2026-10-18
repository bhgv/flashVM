# Compiler development rules

- Implement compilers and frontends in C99. Keep language projects independently
  buildable, including detached/renamed copies. Root orchestration is optional.
- Readable Coco/R .atg files are authoritative scanner/parser sources. Generated
  files go to build directories. Depend explicitly on bhgv/coco-r-C; do not copy
  its documentation, translations, sources or runtime into this repository.
- Document language coverage, ambiguities, limitations and diagnostics in English
  and Russian. Link frontend functions and grammar productions to source files.
- Keep frontend syntax tests separate from semantic, backend and SWF acceptance.
  Never present syntax recognition as a complete compiler or working SWF emitter.
- Each language owns its driver, grammar and tests. Generated globals stay in
  separate executables until a namespaced, reentrant library API is implemented.
- Author and committer: bhgv <bhgv@bhgv.bhgv>. Commit directly on master, version
  commit subjects. Publish only to https://github.com/bhgv/flashVM.git; use full
  URLs rather than assuming origin points at GitHub. Preserve the shared LICENSE.

- Format .atg files for human reading: comment every lexical definition and
  production, explain precedence/associativity and each IF resolver. Split complex
  alternatives into named rules. Document embedded C functions, state and branches.
