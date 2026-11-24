// Copyright (c) 2026 bhgv
// EN: Outline the atlas viewport over the retained live previews.
// RU: Нарисовать рамку области просмотра поверх живых миниатюр карты.


// EN: Keep the viewport interior transparent so the document previews remain visible.
// RU: Оставить середину рамки прозрачной, чтобы были видны миниатюры документов.
function drawDesktopAtlasViewport() {
    desktopAtlasPlaced = _root.placeDesktopItem();
    if (desktopOwner) {
        desktopPane = desktopOwner.createEmptyMovieClip("item" + desktopId, desktopSeen);
        desktopPane._x = desktopX;
        desktopPane._y = desktopY;
        desktopPaint = desktopPane.lineStyle(1, 16764265, 100);
        desktopPaint = desktopPane.moveTo((1 / 2), (1 / 2));
        desktopPaint = desktopPane.lineTo(desktopW - (1 / 2), (1 / 2));
        desktopPaint = desktopPane.lineTo(desktopW - (1 / 2), desktopH - (1 / 2));
        desktopPaint = desktopPane.lineTo((1 / 2), desktopH - (1 / 2));
        desktopPaint = desktopPane.lineTo((1 / 2), (1 / 2));
    }
}
