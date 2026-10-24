# One .m header, external AS module and AS caller of Dis

[Русский](README.ru.md) · [Compiler priority](../../OWN_COMPILER.md) ·
[Generator implementation](../../frontend/bindings.c)

This is bidirectional module scaffolding for Limbo/Dis, not merely translation of
header text into another language. [arithmetic.m](arithmetic.m) is the shared interface:

- A Dis implementation can be [arithmetic.b](arithmetic.b).
- A non-Dis implementation can be [as-implementation.as](as-implementation.as),
  registered through the generated external AS adapter.
- AS callers use [as-caller.as](as-caller.as) with a Dis proxy generated from the SAME header.
- C/Python scaffolds and the neutral descriptor use the same declaration identities.

## Generate from the real Coco/R catalog

```sh
./build-workspace.sh --projects limbo
mkdir -p build/external-modules
build/workspace/limbo/frontend/limbo_bindings --format as-module compiler/limbo/examples/external-modules/arithmetic.m build/external-modules/arithmetic-as.as --prefix arithmetic_as
build/workspace/limbo/frontend/limbo_bindings --format as-proxy compiler/limbo/examples/external-modules/arithmetic.m build/external-modules/arithmetic-dis.as --prefix arithmetic_dis
build/workspace/limbo/frontend/limbo_bindings --format c compiler/limbo/examples/external-modules/arithmetic.m build/external-modules/arithmetic.c --prefix arithmetic_c
build/workspace/limbo/frontend/limbo_bindings --format python compiler/limbo/examples/external-modules/arithmetic.m build/external-modules/arithmetic.py --prefix arithmetic_py
build/workspace/limbo/frontend/limbo_bindings --format json compiler/limbo/examples/external-modules/arithmetic.m build/external-modules/arithmetic.json
```

AS profile spellings `as1-module`, `as2-module`, `as3-module` and corresponding
`*-proxy` are supported. `as`, `as1`, `as2`, `as3` alias module adapters. The current
emitter uses the common untyped subset, syntax-checked by all three real Coco/R
frontends. It does not promise full source lowering/SWF emission or ABC.
The existing catalog owns parsing; no regex or alternate hand parser is added.

## Limbo/Dis calling external AS

Load the generated arithmetic-as.as and handwritten implementation into the AS
side, then obtain `makeArithmeticASModule()`. Its dispatch capability is the AS
implementation of Arithmetic. A native bridge must register it as an external
module whose real resolved signature matches arithmetic.m. A dispatched call has:

```javascript
{schema:1, engine:"as", interface_id:"...", id:7, declaration:1,
 module:"Arithmetic", name:"add", args:[3,4]}
```

The declaration index is read from the generated contract, not assumed to be 1.
The adapter validates endpoint identity, export/name and call shape, invokes its
handler and replies once with `engine:"as"`, the same id/declaration, status,
value/error. Default generated handlers fail as unimplemented. The example's
add wraps with `| 0` to preserve i32 arithmetic; ordinary AS Number addition alone
would differ. The example rejects invalid scalar values visibly.

## AS calling a Dis module

Load arithmetic-dis.as and call `useArithmeticDis(sendToDis)`. The supplied send
capability is a checked host/channel endpoint. The generated proxy resolves
`Arithmetic.add` from .m and sends `engine:"dis"`. The host invokes the real Dis
export and marshals its reply. AS uses callbacks instead of blocking its event
owner. `.callById` supports exact declaration IDs; ambiguous nested short names
are rejected by `.call` rather than silently picking one.

IDs are positive, monotonically allocated within one proxy, capped at 2147483647
and never reused. Replies with wrong schema/engine/header/declaration/id or status
are ignored; duplicates cannot resolve the call twice. Transport failure reports
one error. A send capability must own the endpoint namespace and pending callback
roots; IDs alone are not globally unique across independent proxies. There is no
replay cache or cancellation protocol. Each external dispatch replies once, but
repeating a request is not an exactly-once execution guarantee.

## Native bridge responsibilities and current boundary

The source header snapshot, declarations/types and source positions are retained
in JSON. All target outputs share `interface_id`: FNV-1a64 of original UTF-8 bytes,
with a version prefix. It is a quick mismatch tag, NOT authentication and NOT the
native Dis structural MD5 signature. Hosts must resolve/compare the full descriptor,
imports/aliases/ADT layouts and actual runtime signatures before registration.
Whitespace/header edits change the tag, so regenerate both sides together.

The host must validate argument/result arity/types/ranges and marshal strings,
records, references, channels and resources as owned values or typed capabilities.
AS Number is not a lossless Limbo big; raw pointers/Dis frames must never be copied
into SWF objects. Preserve GC roots for pending calls, retire them on completion,
and report boundary frames to the debugger. Top-level argument arrays are copied
by the proxy; complex values still require host-controlled snapshot/serialization.
C scaffolds use opaque call objects and explicitly return -1 until implemented;
they do not invent native struct layouts. Includes are cataloged, not opened.

The generated adapters are executable common-AS source and a transport contract.
A complete native Dis/SWF loader/marshaler and full AS source compiler are still
under development. No actual cross-VM execution is claimed here. Tests compile/run
C scaffolds, check AS through three Coco/R parsers and optionally execute the
actual common-AS output in Node against a Dis transport double. Node is a test
runtime only, not a deployed dependency or proof of SWF execution.

[Argument/result guards and function bindings / Проверки аргументов и результата](../../frontend/SIGNATURES.md)
