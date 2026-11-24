// Copyright (c) 2026 bhgv
// EN: Attach a fitted, clipped raster of the staged source to the mirror holder.
// RU: Добавить масштабированный и обрезанный растр исходного клипа в зеркало.


// EN: Fit a real clip in its card. The snapshot surface clips any scrolled portion.
// RU: Вписать настоящий клип в карточку. Растр обрезает вышедшую за полку часть.
function paintDesktopMirrorPixels() {
    desktopMirrorResolved = _root.resolveDesktopMirror();
    if (desktopMirrorSource) {
        desktopMirrorFit = Math.min(desktopMirrorWidth / desktopSourceWidth, desktopMirrorHeight / desktopSourceHeight);
        desktopMirrorFit = Math.min(1, desktopMirrorFit);
        desktopMirrorShiftX = (desktopW - desktopMirrorFit * desktopSourceWidth) / 2;
        desktopMirrorShiftY = desktopMirrorTop - desktopMirrorCrop
            + (desktopMirrorHeight - desktopMirrorFit * desktopSourceHeight) / 2;
        desktopMirrorPixels = desktopMirrorSource.snapshotBitmap(desktopW, desktopH, desktopMirrorFit,
            desktopMirrorFit, desktopMirrorShiftX, desktopMirrorShiftY);
        if (desktopMirrorPixels) {
            desktopMirrorAttached = desktopPane.attachBitmap(desktopMirrorPixels, 1, "never", false);
        }
    }
}
