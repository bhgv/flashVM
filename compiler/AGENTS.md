# Compiler development rules

- Implement compilers and frontends in C99. Keep language projects independently
  buildable, including detached/renamed copies. Root orchestration is optional.
- Readable Coco/R .atg files are authoritative scanner/parser sources. Generated
  files go to build directories. Detached projects take explicit Coco/R SDK paths. Root builds use selected
  owned sources under tools/coco-r-C or tools/coco-r-limbo. Preserve manifests,
  upstream Git addresses/revisions, notices and byte-for-byte upstream files.
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

- Prioritize our Coco/R Limbo pipeline over the legacy native bootstrap. Compile
  debug origins and generate external module scaffolds from .m interfaces,
  including C and AS for the planned complementary Dis/SWF execution model.
- Confirmed upstream defects must be fixed in their owning repository/work branch
  and exported as an upstream patch; refresh selected snapshots from that commit.
  Do not hide such fixes as untracked edits to vendored upstream files.
