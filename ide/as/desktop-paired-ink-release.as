// Copyright (c) 2026 bhgv
// EN: Freeze a gesture before publishing its protocol fragments.
// RU: Зафиксировать жест перед публикацией его фрагментов.


// EN: Reserve the final logical point (last inside the page if released outside).
// Keep the preview until a replacement scene is published.
// RU: Сохранить последнюю точку внутри листа при отпускании за его пределами.
// Оставить предпросмотр до публикации следующего снимка.
function releaseDesktopInk() {
    if (!desktopInkInside) {
        desktopPointerOffset = desktopInkLastPoint;
    }
    desktopInkSampled = _root.appendDesktopInkSample();
    if (desktopInkInside) {
        desktopInkPainted = _root.paintDesktopInkSample();
    }
    desktopInkSending = true;
    desktopInkSent = 0;
    desktopReady = false;
    desktopInkQueued = _root.queueDesktopInkSend();
}
