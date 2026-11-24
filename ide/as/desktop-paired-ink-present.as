// Copyright (c) 2026 bhgv
// EN: Invalidate the local FlashVM raster while keeping the Dis ticket.
// RU: Обновить локальный растр FlashVM с сохранением билета Dis.


// EN: Publish a presentation-only revision on the sole MovieClip owner.
// RU: Опубликовать ревизию отображения у единственного владельца MovieClip.
function publishDesktopInk() {
    desktopPresentation = desktopPresentation + 1;
    desktopInkPublished = hostInput.publishFrame(desktopPresentation);
}
