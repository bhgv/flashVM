// Copyright (c) 2026 bhgv
// EN: Carry an expected namespace state from the Limbo test over StyxRPC.
// RU: Передать ожидаемое состояние пространства имён из проверки Limbo по StyxRPC.


// EN: Save a phase number, expected current byte and visible-directory mask, then acknowledge.
// RU: Сохранить номер фазы, ожидаемый байт и маску списка каталога, затем подтвердить.
function rebindSignal(operation, bytes, size) {
    _root.rebindPhase = bytes[0];
    _root.rebindByte = bytes[1];
    _root.rebindNames = bytes[2];
    return 0;
}
