# StyxexecHandler.invoke

[Русский](styx-exec-invoke.ru.md) · [Guide](STYX-EXEC-PUBLISH.md) · [Interface](../module/styxexec.m#L8) · [Implementation](../../lib/styxrpc/tests/flashvm/publish_handler.b)

```limbo
invoke(path: string, bytes: array of byte): array of byte
```

Implement this method in a module passed to Node.dispatch. It receives the absolute published path and owned request bytes on the selected Limbo thread. Return an Array no longer than the input, or nil for a successful empty reply. Raise to refuse the invocation.

```limbo
return bytes;
```
