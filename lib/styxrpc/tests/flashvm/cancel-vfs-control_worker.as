// Copyright (c) 2026 bhgv
// EN: Enter one exact VFS control wait using the detached task's mounted session.
// RU: Войти в одно ожидание VFS через смонтированную сессию отделённой задачи.


// EN: Borrow the stage number and run its VFS operation. Close any successful handle.
// RU: Принять номер этапа и выполнить его операцию VFS. Закрыть выданный дескриптор.
asinc function controlWait(stage) {
    if (!stage) {
        var writer = os.open("/created-by-cancel", "wx");
        var closed = writer.close();
    }
    if (!(stage - 1)) {
        var directory = os.opendir("/hold-dir");
        var item = directory.next_file();
        var retired = directory.close();
    }
    if (!(stage - 2)) {
        var removed = vfs.rm("/remove-by-cancel");
    }
    if (!(stage - 3)) {
        var info = os.attr("/attr-cancel");
    }
    if (!(stage - 4)) {
        var reader = os.open("/open-cancel");
        var released = reader.close();
    }
    return stage;
}
