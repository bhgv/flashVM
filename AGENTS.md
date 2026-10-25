# Repository-wide source readability

The user's current readability requirements apply to handwritten implementation,
headers, tests and examples in every component. They supersede older local
exemptions from readability work. Keep upstream snapshots, third-party libraries,
generated parser outputs and deliberately raw oracle fixtures unchanged.

- Do not use assert. Use an explicit if with an error message or appropriate
  recovery/failure action; preserve evaluation and required calls in conditions.
- Use braces for every controlled body, even one statement. Do not combine
  independent statements on one physical line. Loop-header separators are syntax.
- Use four-space indentation. Leave a blank after the include group and before
  public/protected/private labels. Leave TWO blank lines before documentation
  attached to a function/method, struct, enum or class declaration/definition.
- Before each function/method, explain purpose, every parameter, the return value,
  ownership, errors and blocking/thread restrictions when relevant. Do not infer
  nonexistent guarantees from a function name or generate filler comments.
- Explain global variables, defines, enum variants, fields and members in trailing
  comments on their declaration lines. Align trailing comments within declaration
  groups. Use a separate declaration line for each field/member.
- Follow tools/readability/README.md and the root/component .clang-format profile.
  Formatting tools are optional development tools, never new build prerequisites.
- Preserve C89 public/C++98 FlashVM and C99 compiler/host compatibility. A layout
  pass must preserve literals, native token order and raw test/oracle contents.
- Never claim automatic formatting verifies the semantic quality/completeness of
  comments. Review contracts against implementations and run affected checks.
- Prepare and test on local work branches; keep downloadable backup patches.
  The user has explicitly authorized publication to github.com/bhgv/flashVM.
  Verify the destination, expected branch head, author bhgv and final tree; never
  force-push. Local author/committer: bhgv <bhgv@bhgv.bhgv>.

- Write Coco/R in human-facing prose. Preserve existing executable, path and
  variable identifiers such as `cocor` and COCOR_ROOT. The CMake generator tests
  automatically check SDK selection and Markdown prose spelling.
