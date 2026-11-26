# vfs.mkdir(path)

[English](STYX_VFS_MKDIR.md) · [Файловый API](STYX_RPC_OS.ru.md) · [Реализация](../src/styxrpc/session_paths.c) · [API C](../../lib/styxrpc/reference/vfs_mkdir.ru.md)

`vfs.mkdir(path)` и `os.mkdir(path)` создают один каталог и возвращают
`true` после подтверждения. Относительный путь начинается от общего
[рабочего каталога](STYX_VFS_CD.ru.md). Родитель должен существовать и
разрешать создание. Занятое имя, скрытый служебный путь и отказ провайдера
дают ошибку. Вызов ждёт ответа Styx. В штатной паре DisVM применяет права
0777 с учётом umask процесса. При неопределённом результате проверьте
текущий каталог перед повтором.

```actionscript
var made = vfs.mkdir("drafts");
var entered = os.cd("drafts");
```
