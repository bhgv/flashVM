// Copyright (c) 2026 bhgv
// EN: Convert player events into the ordinary desktop input envelope.
// RU: Преобразовать события плеера в штатные сообщения ввода desktop.


// EN: Encode one copied signed or unsigned 32-bit word as lowercase hexadecimal.
// RU: Закодировать одно скопированное 32-битное слово строчными цифрами.
function encodeDesktopWord() {
    // Number.toString(radix) in a loaded movie formats signed 32-bit values.
    // В загруженном фильме Number.toString(radix) форматирует знаковый int32.
    desktopWordHigh = Math.floor(desktopWord / 65536);
    desktopWordLow = desktopWord - desktopWordHigh * 65536;
    desktopWordNonnegative = Math.max(0, desktopWordHigh);
    if (desktopWordHigh - desktopWordNonnegative) {
        desktopWordHigh = desktopWordHigh + 65536;
    }
    desktopHexHigh = "0000" + desktopWordHigh.toString(16);
    desktopHexLow = "0000" + desktopWordLow.toString(16);
    desktopHexWord = desktopHexHigh.substr(0 - 4) + desktopHexLow.substr(0 - 4);
}
