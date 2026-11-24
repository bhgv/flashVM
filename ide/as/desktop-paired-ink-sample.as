// Copyright (c) 2026 bhgv
// EN: Store one logical point as five hexadecimal digits.
// RU: Хранить логическую точку пятью шестнадцатеричными цифрами.


// EN: Append one admitted page point while keeping the final sample slot free.
// RU: Добавить допущенную точку листа, оставляя место последней точке.
function appendDesktopInkSample() {
    desktopInkLastPoint = desktopPointerOffset;
    desktopInkHexPoint = "00000" + desktopInkLastPoint.toString(16);
    desktopInkPoints = desktopInkPoints + desktopInkHexPoint.substr(0 - 5);
    desktopInkCount = desktopInkCount + 1;
}
