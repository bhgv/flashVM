# Node.close

[Русский](styx-exec-close.ru.md) · [Guide](STYX-EXEC-PUBLISH.md) · [Interface](../module/styxexec.m#L42) · [Implementation](../limbo/styxexec.b#L196)

```limbo
node.close()
```

Release the owner FD. Native retirement withdraws its exact namespace layer and wakes a pending call with refusal. Calling close again is harmless. Dropping the last FD reference also retires the owner. Serialize close with the Node's other operations.

```limbo
node.close();
```
