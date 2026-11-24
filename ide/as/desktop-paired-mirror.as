// Copyright (c) 2026 bhgv
// EN: Stage one shelf or overview mirror bearing its original layout identity.
// RU: Подготовить зеркало полки или карты с его исходным идентификатором компоновки.


// EN: Clip scrolling shelf cards to their visible rail and preserve hit geometry.
// RU: Обрезать карточки прокрутки по видимой полке и сохранить геометрию попаданий.
function drawDesktopMirror() {
    desktopShelfMirror = !(desktopParent - 2);
    desktopShelfTitle = false;
    desktopMirrorWidth = desktopW;
    desktopMirrorHeight = desktopH;
    desktopMirrorTop = 0;
    desktopMirrorCrop = 0;
    if (desktopShelfMirror) {
        desktopMirrorWidth = desktopW - 6;
        desktopMirrorHeight = desktopH - 23;
        desktopMirrorTop = 19;
        desktopMirrorVisibleY = Math.max(72, desktopY);
        desktopMirrorCrop = desktopMirrorVisibleY - desktopY;
        desktopH = Math.min(desktopHeight, desktopY + desktopH) - desktopMirrorVisibleY;
        desktopY = desktopMirrorVisibleY;
        if (!desktopMirrorCrop) {
            desktopShelfTitle = !(Math.max(18, desktopH) - desktopH);
        }
    }
    if (Math.max(0, desktopH)) {
        desktopMirrorPlaced = _root.placeDesktopItem();
        if (desktopOwner) {
            desktopPane = desktopOwner.createEmptyMovieClip("item" + desktopId, desktopSeen);
            desktopPane._x = desktopX;
            desktopPane._y = desktopY;
            desktopMirrorCard = _root.paintDesktopMirrorCard();
            desktopMirrorImage = _root.paintDesktopMirrorPixels();
        }
    }
}
