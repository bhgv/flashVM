# Node.dispatch

[Русский](styx-exec-dispatch.ru.md) · [Guide](STYX-EXEC-PUBLISH.md) · [Interface](../module/styxexec.m#L39) · [Implementation](../limbo/styxexec.b#L175)

```limbo
node.dispatch(handler: StyxexecHandler): int
```

Borrow an initialized handler module and receive one request on the Node. Invoke the handler on the calling Limbo thread. An exception or a result longer than the input causes explicit refusal. Return 1 if the reply or refusal was accepted or 0 after its deadline. A missing handler or failed receive raises.

```limbo
settled := node.dispatch(handler);
```
