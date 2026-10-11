// API illustration; executable coverage is in tests/actionscript_error.cpp.
// Иллюстрация API; исполняемая проверка находится в tests/actionscript_error.cpp.
try {
    throw new Error("operation failed");
} catch (failure) {
    if (failure instanceof Error) {
        trace(failure.toString());
    }
} finally {
    trace("completed");
}
