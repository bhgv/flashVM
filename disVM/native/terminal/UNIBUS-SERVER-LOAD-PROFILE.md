# styxunibusserverloadprofile

[Русский](UNIBUS-SERVER-LOAD-PROFILE.ru.md) · [Header](unibus_server.h) · [Implementation](unibus_loaded.c)

```c
DVM_Provider *styxunibusserverloadprofile(StyxUnibusServer *server);
```

Include `unibus_server.h` and link `styx_unibus_server`. Pass the live
server returned by [styxunibusserverstartloaded](UNIBUS-SERVER-LOADED.md).
The call returns immediately with a provider borrowed until join. It
returns NULL for other server modes or a NULL server.

Grant only write access to the provider's root node in the owner's VFS.
Pass the grant path as the `offer` field of the `loaded` configuration.
The owner writes one complete S-record after resolving the export. The
dispatcher verifies the selected name and signature and seals the
mailbox when it reads that record. Remote clients read the accepted
profile from `.interface` on the mounted directory.
