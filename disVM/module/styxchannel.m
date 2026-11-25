# Copyright (c) 2026 bhgv
# EN: Typed messages on a mixed UniBus channel attached to DisVM.
# RU: Типизированные сообщения на смешанном канале UniBus текущего DisVM.
Styxchannel: module
{
    PATH: con "styxchannel.dis"; # Installed portable Limbo client.

    Message: adt
    {
        pick {
            Integer =>
                value: int; # Signed Limbo Int32 snapshot.
            Floating =>
                value: real; # Binary64 snapshot.
            Boolean =>
                value: int; # Zero or one.
            Text =>
                value: string; # Counted string.
            Null =>
                reserved: int; # Zero, carries no wire payload.
            Variable =>
                cell: ref Styxintref->Cell; # Original typed lease, resolved on the issuing mount.
        }
    };

    Port: adt
    {
        handle: int; # Private launch-scoped native lease.
        root: string; # Absolute path of the issuing peer mount, for reference resolution.

        # EN: Send a message, returning 1 on success, 0 for backpressure.
        # RU: Отправить сообщение. 1 при успехе, 0 если очередь занята.
        send: fn(port: self ref Port, message: ref Message): int;

        # EN: Receive one complete typed message or nil when pending.
        # RU: Принять одно полное типизированное сообщение либо nil при ожидании.
        recv: fn(port: self ref Port): ref Message;

        # EN: Release this port. Calling close twice is harmless.
        # RU: Освободить порт. Повторное закрытие безопасно.
        close: fn(port: self ref Port);
    };

    # EN: Load the mounted-session bindings. Call once before opening ports.
    # RU: Загрузить привязки смонтированной сессии. Вызвать до открытия портов.
    init: fn();

    # EN: Open address on the paired Styx session. root is its trusted mount point.
    # RU: Открыть address в парной сессии Styx. root — её точка монтирования.
    newchan: fn(address, root: string): ref Port;
};
