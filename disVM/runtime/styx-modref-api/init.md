# Styxmodref.init

[Русский](init.ru.md) · [Declaration](../../module/styxmodref.m) · [Implementation](../../limbo/styxmodref.b) · [Guide](../STYX-MODULE-REFS.md)

`styxmodref.dis`

```limbo
init: fn();
```

Load the Sys binding before opening cells. Call `refs->init()` after loading the module and before sharing the instance with other Limbo threads. The call does not mount a peer. The paired runtime already provides the reference service.
