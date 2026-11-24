// Copyright (c) 2026 bhgv
// EN: Map ordinary pointer events using the published drawing and parent geometry.
// RU: Преобразовать события указателя по опубликованной геометрии рисунка и родителя.


// EN: Decode signed pixel origins from a borrowed pair of eight-digit words.
// RU: Декодировать знаковые координаты из заимствованной пары восьмизначных слов.
function readDesktopVectorOrigin() {
    desktopVectorOriginX = ("0x" + desktopVectorOrigin.substr(0, 8)) * 1;
    desktopVectorOriginY = ("0x" + desktopVectorOrigin.substr(8, 8)) * 1;
    if (desktopSign.indexOf(desktopVectorOrigin.substr(0, 1)) + 1) {
        desktopVectorOriginX = desktopVectorOriginX - 65536 * 65536;
    }
    if (desktopSign.indexOf(desktopVectorOrigin.substr(8, 1)) + 1) {
        desktopVectorOriginY = desktopVectorOriginY - 65536 * 65536;
    }
}
