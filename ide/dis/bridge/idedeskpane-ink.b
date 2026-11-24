# Copyright (c) 2026 bhgv
# EN: Validate and adopt a released portable stroke inside its document owner.
# RU: Проверить и принять завершённый штрих внутри владельца документа.

InkMotionWindowMs: con 40; # Minimum elapsed time for an interior brush anchor.


# EN: Decode a fixed field of lowercase ASCII hex; -1 means malformed data.
# RU: Прочитать поле строчных шестнадцатеричных цифр ASCII. Ошибка даёт -1.
inkhex(bytes: array of byte, start, width: int): int
{
    number := 0;
    for (digit := start; digit < start + width; digit++) {
        value := int bytes[digit];
        if (value >= '0' && value <= '9') {
            value -= '0';
        } else if (value >= 'a' && value <= 'f') {
            value = value - 'a' + 10;
        } else {
            return -1;
        }
        number = 16 * number + value;
    }
    return number;
}


# EN: Compact validated points in place and return a prefix of the same array.
# Average each interior time window on both axes independently. The exact
# press and release own their adjacent windows and always survive as anchors.
# RU: Уплотнить проверенные точки на месте и вернуть часть того же массива.
# Усреднить каждое внутреннее окно по обеим осям. В окнах рядом с нажатием
# и отпусканием сохраняются точные координаты этих событий.
smoothtimedink(points, times: array of int): array of int
{
    last := len points - 1;
    kept := 1;
    first := 1;
    while (first < last && times[first] < InkMotionWindowMs) {
        first++;
    }
    while (first < last && times[last] - times[first] >= InkMotionWindowMs) {
        until := times[first] + InkMotionWindowMs;
        count := 0;
        x := 0;
        y := 0;
        while (first < last && times[first] < until && times[last] - times[first] >= InkMotionWindowMs) {
            point := points[first++];
            x += point % 1001;
            y += point / 1001;
            count++;
        }
        points[kept++] = (x + count / 2) / count + 1001 * ((y + count / 2) / count);
    }
    points[kept++] = points[last];
    return points[:kept];
}


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
    if (e == nil || e.kind != Uibridge->Request || e.flags != doc.revision || len doc.points != 0 ||
        (doc.tool != Idevector->Brush && doc.tool != Idevector->Pen)) {
        return Idecore->Conflict;
    }
    width := 5;
    if (e.first == 1) {
        width = 11;
    }
    count := len bytes / width;
    if (count < 2 || count > Idevector->MaxNodes || len bytes != width * count) {
        return Idecore->Invalid;
    }
    points := array[count] of int;
    times: array of int;
    if (width == 11) {
        times = array[count] of int;
    }
    elapsed := 0;
    for (i := 0; i < count; i++) {
        point := inkhex(bytes, width * i, 5);
        if (point < 0 || point > 601600) {
            return Idecore->Invalid;
        }
        points[i] = point;
        if (times != nil) {
            time := inkhex(bytes, width * i + 5, 6);
            if (time < elapsed || (i == 0 && time != 0)) {
                return Idecore->Invalid;
            }
            times[i] = elapsed = time;
        }
    }
    if (times != nil && doc.tool == Idevector->Brush) {
        points = smoothtimedink(points, times);
        count = len points;
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
