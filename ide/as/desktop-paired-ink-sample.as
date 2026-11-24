// Copyright (c) 2026 bhgv
// EN: Store one logical point and its elapsed pointer time.
// RU: Хранить логическую точку и время указателя с начала мазка.


// EN: Append a page point and six hex digits of nondecreasing elapsed milliseconds.
// RU: Добавить точку листа и шесть цифр неубывающего времени в миллисекундах.
function appendDesktopInkSample() {
    desktopInkLastPoint = desktopPointerOffset;
    desktopInkHexPoint = "00000" + desktopInkLastPoint.toString(16);
    desktopInkDelta = desktopPointerTime - desktopInkStartTime;
    desktopInkClockRange = 65536 * 65536;
    desktopInkDelta = desktopInkDelta - desktopInkClockRange * Math.floor(desktopInkDelta / desktopInkClockRange);
    desktopInkElapsed = Math.max(desktopInkElapsed, Math.min(16777215, desktopInkDelta));
    desktopInkHexTime = "000000" + desktopInkElapsed.toString(16);
    desktopInkPoints = desktopInkPoints + desktopInkHexPoint.substr(0 - 5) + desktopInkHexTime.substr(0 - 6);
    desktopInkCount = desktopInkCount + 1;
}
