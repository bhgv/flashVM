# Contextual `asinc` source fixture

[Русский](README.ru.md) · [Contract/status](../../ASINC.md)

[captured.as](captured.as) is fully commented and accepted by the generated
AS1/AS2/AS3 **syntax** frontends. It illustrates a named function, a block, lexical
references and an expression handle. It is not an executable SWF/Dis example:
closure/task lowering, capture cells and task results are still pending.

```sh
as1_parse captured.as
as2_parse captured.as
as3_parse captured.as
fir1_source as2 --input captured.as output.fir --diagnostics-json
```

The first three commands succeed. The last must fail explicitly with a semantic
`lexical-closure/task lowering` diagnostic at the first `asinc` prefix and preserve
any existing output.fir. It must not silently run the block synchronously.

For an actually executed frame-free drawing example, build/run
[the C regression](../../../disVM/runtime/test_frame_free_drawing.c) as described
in the contract. That uses the already working Async.post owner-loop API and
independent raster commands, not the unimplemented source extension.
