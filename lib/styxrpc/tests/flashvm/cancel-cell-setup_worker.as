// Copyright (c) 2026 bhgv
// EN: Interrupt each phase of a named shared cell before exercising the same identity again.
// RU: Прервать каждую фазу общей ячейки и затем снова обратиться к той же ячейке.


// EN: Borrow stage. Attach a cell, then read or replace a value at the selected phase.
// Return 42 from recovery after observing the cancelled assignment's accepted value.
// RU: Принять stage. Присоединить ячейку и выполнить операцию этапа.
// При восстановлении вернуть принятое значение отменённого присваивания 42.
asinc function setupOperation(stage) {
    var answer = 0;
    var port = os.chan("/cell-" + stage);
    var alias = port.recv();
    if (!(stage - 1)) {
        answer = alias + 0;
    }
    if (!(stage - 2)) {
        answer = alias + 0;
    }
    if (!(stage - 3)) {
        alias = 99;
    }
    if (!(stage - 4)) {
        alias = 42;
    }
    if (!(stage - 5)) {
        answer = alias + 0;
        alias = 73;
    }
    var released = port.delete();
    return answer;
}
