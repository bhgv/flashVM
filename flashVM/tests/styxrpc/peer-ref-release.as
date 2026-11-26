// Copyright (c) 2026 bhgv
// EN: Release the provider's readiness marker over its attached StyxRPC session.
// RU: Разрешить публикацию признака готовности через подключённую сессию StyxRPC.


// EN: Accept the execute request and its borrowed buffer, returning no result bytes.
// RU: Принять запрос выполнения и его буфер, не возвращая байтов результата.
function releaseRefPeer(operation, buffer, size) {
    _root.refPublishReady = true;
    return 0;
}
