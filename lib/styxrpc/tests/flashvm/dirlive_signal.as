// Copyright (c) 2026 bhgv
// EN: Let Limbo schedule directory mutations between reads.
// RU: Позволить Limbo запланировать изменения каталога между чтениями.


// EN: Record the phase byte on the movie root and confirm an empty reply.
// RU: Записать байт фазы в корень фильма и подтвердить пустой ответ.
function changeDirectoryPhase(op, buffer, size) {
    _root.directoryPhase = buffer[0];
    return 0;
}
