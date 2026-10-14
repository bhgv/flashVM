# AVM1 With scopes

v0.233 introduces `fvm_exec_with_scope` and `fvm_exec_scope_lookup` in
`flashvm/execution.h`. Their C89 interfaces accept managed objects and values;
they neither decode Actions nor access an interpreter operand stack. Generated
code can create the same scopes, resolve names and retain them as ordinary GC
roots. Keyword `this`, display targets and global fallback remain caller policy.

Each private With node retains its object in managed state and its enclosing
lexical scope through its prototype edge. Function creation captures the active
node. An activation takes precedence over its captured scopes; a With entered
inside that activation takes precedence over its locals. A found undefined
binding remains distinguishable from an absent name. Lookup returns its binding
object for assignment and With method receivers. Accessors execute with that
object. Primitive scopes are boxed; null and undefined skip their body.

The interpreter owns only record bounds, operand consumption and control flow.
The decoder checks the UI16 body length and inspects nested actions. Blocks share
registers, constants, stack and execution budget; return and throw propagate.
Every exit restores the lexical root chain. Private resource kind 38 is added to
the existing graph schema: current readers preserve captured scopes through
channels, older readers reject this newly supported kind. No pointer or native
body attachment is serialized.

The unchanged original this_swf5/6/7 SWFs now produce byte-exact traces with zero
exit and empty stderr. The selected original corpus reaches 21/22; the remaining
object_prototypes mismatch concerns prototype property membership and casing.
The new fixture covers returned closures, GC, graph transfer, lookup/assignment,
undefined bindings, primitive boxing, null scopes, malformed records and throws.
Allocation-failure injection verifies scope output and root-chain restoration.
These checks do not close any roadmap stage. General property casing, full scope
path syntax, historical primitive boxing and captured base-clip semantics remain
separate compatibility work.

Validation: Linux AGG 435/435; Cairo Release 434/434 before adding the independent
allocation fixture (that fixture passed in AGG); five focused heap/global
ASan+UBSan tests passed. Stack and leak instrumentation remain disabled.
