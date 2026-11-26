// Copyright (c) 2026 bhgv
// EN: Start the named transfer after the caller has opened its ordinary variable fids.
// RU: Запустить обмен после открытия вызывающим обычных fid переменной.


// EN: Acknowledge the caller's write and let the next owner frame receive refs.
// RU: Подтвердить запись вызывающего и разрешить приём ссылок со следующего кадра.
function startModuleRefs(operation, bytes, size) {
    _root.moduleRefReady = true;
    return size;
}
