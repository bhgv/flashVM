// Copyright (c) 2026 bhgv
// EN: Exercise the same native Int32 through fresh named references at each phase.
// RU: Проверить одну нативную переменную Int32 через новые именованные ссылки на каждом этапе.


// EN: Read the mounted value or replace it at the chosen stage. Recovery expects
// the unacknowledged write and sets a distinct value through the same publication.
// RU: Прочитать смонтированное значение или заменить его на выбранном этапе.
// После отмены прочитать неподтверждённую запись и установить другое значение.
asinc function setupOperation(stage) {
    var port = os.chan("/int-ref-" + stage);
    var alias = port.recv();
    var answer = 0;
    if (stage - 4) {
        answer = alias + 0;
        if (!(stage - 5)) {
            alias = 73;
        }
    } else {
        alias = 42;
    }
    var released = port.delete();
    return answer;
}
