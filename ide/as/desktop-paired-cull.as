// Copyright (c) 2026 bhgv
// EN: Hide whole off-viewport table items from the window raster.
// RU: Исключить целиком вышедшие за область просмотра элементы из растра окна.


// EN: Use the staged table transform. Keep the local clip available to its mirrors.
// RU: Использовать матрицу стола снимка, сохраняя локальный клип для его зеркал.
function cullDesktopTableItem() {
    desktopPaintX = desktopX * desktopScale + desktopTableX;
    desktopPaintY = desktopY * desktopScale + desktopTableY;
    desktopVisibleWidth = Math.min(desktopWidth, desktopPaintX + desktopW * desktopScale) - Math.max(0, desktopPaintX);
    desktopVisibleHeight = Math.min(desktopHeight, desktopPaintY + desktopH * desktopScale) - Math.max(48, desktopPaintY);
    desktopItemVisible = Math.max(0, desktopVisibleWidth) * Math.max(0, desktopVisibleHeight);
}
