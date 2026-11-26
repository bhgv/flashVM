# inspect

[Русский](inspect.ru.md) · [Guide](../STYX-NAMED-VALUES.md) · [Declaration](../../module/styxvalue.m#L59) · [Implementation](../../limbo/styxvalue_graph_read.b#L219)

```limbo
inspect(bytes: array of byte): ref DataGraph
```

Read portable own-data records of an FVMG v8/v9 envelope excluding the named tag. Return a graph with a root value and indexed nodes. Unsupported representations return nil, while corruption raises. Borrow bytes during parsing. The returned data can be edited independently. This view carries no runtime callbacks. No I/O occurs.

```limbo
pick envelope := value { Graph => graph = codec->inspect(envelope.bytes); }
```
