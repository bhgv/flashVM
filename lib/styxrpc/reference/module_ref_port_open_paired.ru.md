# styxrpc_module_ref_port_open_paired

[English](module_ref_port_open_paired.md) · [Объявление](../include/styxrpc/module_ref_port.h#L36) · [Реализация](../src/module_ref_port.c#L36) · [Руководство](module_ref_port.ru.md)

`StyxRpc::launch`

```c
StyxRpcModuleRefPort *styxrpc_module_ref_port_open_paired(struct StyxClient *issuer, unsigned owner,
    const void *scope, StyxRpcModuleRefLocalOpen local, void *context);
```

Передайте обычное подключение в `issuer`, его роль в `owner`
(`STYXRPC_MODULE_REF_FORWARD` или `STYXRPC_MODULE_REF_REVERSE`) и
согласованные 16 байтов в `scope`. Порт копирует область, удерживает
клиента, заимствует необязательный колбэк `local` и `context` до
закрытия. Если `local` задан, приём допускает встречную роль и при первом
обращении к значению вызывает этот колбэк. Захват использует обычное
подключение к владельцу. NULL в `local` даёт поведение [open](module_ref_port_open.ru.md).

Колбэк получает `context`, проверенный дескриптор и обнулённый
`StyxRpcFile` для результата. Сверьте точный путь, Qid и запрошенный
режим. При `STYXRPC_IO_OK` передайте один открытый файл с разрешённым
доступом. При ошибке верните соответствующий код и оставьте результат
пустым. Открытый файл должен удерживать тот же узел до освобождения.
Его операции VFS сообщают ошибки местного I/O и не повторяют запись.
Разрешение ссылки и местные операции могут ждать владельца.

При создании порт не выполняет запросов Styx. Вызов возвращает
собственный пустой порт либо NULL при неверных аргументах, отказе
выделения памяти или удержания. Выполняйте операции с каждым портом
последовательно. Используйте тот же контракт
[записи](module_ref_port_write.ru.md), [чтения](module_ref_port_read.ru.md)
и [закрытия](module_ref_port_close.ru.md), что и у порта с одним удалённым
владельцем.

[Адаптер смешанной пары DisVM](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/terminal_launch/module_refs.c)
использует прямое подключение к владельцу и проверяет встречные Qid в
своих местных публикациях.
