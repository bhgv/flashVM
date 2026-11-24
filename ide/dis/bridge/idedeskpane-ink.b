# Copyright (c) 2026 bhgv
# EN: Validate and adopt a released portable stroke inside its document owner.
# RU: Проверить и принять завершённый штрих внутри владельца документа.


# EN: Read all copied packed points before starting any document gesture.
# The caller owns doc and the request; accepted samples go through the same
# Brush/Pen model and history as ordinary motion, with one release.
# RU: Проверить все упакованные точки до начала жеста. Документ и запрос
# принадлежат владельцу панели. Точки проходят через штатную модель кисти
# или пера и попадают в историю при единственном отпускании кнопки.
portablestroke(vector: Idevector, doc: ref Idevector->Document, request: Request, deferred: int): int
{
    e := request.event;
    bytes := array of byte request.text;
    count := len bytes / 5;
    if (e == nil || e.kind != Uibridge->Request || e.flags != doc.revision || len doc.points != 0 ||
        (doc.tool != Idevector->Brush && doc.tool != Idevector->Pen)) {
        return Idecore->Conflict;
    }
    if (count < 2 || count > Idevector->MaxNodes || len bytes != 5 * count) {
        return Idecore->Invalid;
    }
    points := array[count] of int;
    for (i := 0; i < count; i++) {
        point := 0;
        for (digit := 0; digit < 5; digit++) {
            value := int bytes[5 * i + digit];
            if (value >= '0' && value <= '9') {
                value -= '0';
            } else if (value >= 'a' && value <= 'f') {
                value = value - 'a' + 10;
            } else {
                return Idecore->Invalid;
            }
            point = 16 * point + value;
        }
        if (point > 601600) {
            return Idecore->Invalid;
        }
        points[i] = point;
    }
    for (i = 0; i < count; i++) {
        phase := 2;
        if (i == 0) {
            phase = 1;
        } else if (i == count - 1) {
            phase = 3;
        }
        point := points[i];
        status: int;
        if (deferred) {
            status = vector->defer(doc, phase, point % 1001, point / 1001);
        } else {
            status = vector->pointer(doc, phase, point % 1001, point / 1001);
        }
        if (status != Idevector->Ok) {
            if (i > 0) {
                vector->cancel(doc);
            }
            return vectorstatus(status);
        }
    }
    return Idecore->Ok;
}
