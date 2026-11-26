# build

[Русский](build.ru.md) · [Guide](../STYX-NAMED-VALUES.md) · [Declaration](../../module/styxvalue.m#L63) · [Implementation](../../limbo/styxvalue_graph_write.b#L280)

```limbo
build(graph: ref DataGraph): ref Value
```

Encode reachable records of a borrowed DataGraph as an owned Value.Graph in FVMG v8. Field names and sparse keys must be unique. Links use zero-based indices into nodes. Cycles and shared nodes are supported. Raises for missing targets, out-of-profile records or a snapshot beyond 65,535 bytes before a named tag. No I/O occurs.

```limbo
port.sendvalue(ref Message.Data(codec->build(graph)));
```
