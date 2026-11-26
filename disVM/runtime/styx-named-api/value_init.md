# Styxvalue.init

[Русский](value_init.ru.md) · [Guide](../STYX-NAMED-VALUES.md) · [Declaration](../../module/styxvalue.m#L67) · [Implementation](../../limbo/styxvalue.b#L19)

```limbo
init()
```

Initialize the loaded Styxvalue instance before using the codec. Loads the runtime Math module for exact binary64 conversions. A missing Math module raises. Call before sharing the module instance across Dis threads.

```limbo
codec->init();
```
