# Styxexec.publish

[Русский](styx-exec-publish.ru.md) · [Guide](STYX-EXEC-PUBLISH.md) · [Interface](../module/styxexec.m#L47) · [Implementation](../limbo/styxexec.b#L107)

```limbo
publish(path: string): ref Node
```

Call after init. Pass a canonical absolute UTF-8 path of at most 1023 bytes including the leading slash. Its parent must already exist and permit write and traversal. Reserved paths and existing entries are refused with an exception. Success returns the owned Node on this paired namespace.

```limbo
node := exec->publish("/tools/apply");
```
