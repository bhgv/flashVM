// Copyright (c) 2026 bhgv
// EN: Release the transitory stroke display on cancellation or scene replacement.
// RU: Снять временное отображение при отмене или смене снимка.


// EN: Drop the live overlay. Any released samples belong to their own send.
// RU: Убрать живое наложение. Уже отпущенные точки принадлежат своей отправке.
function discardDesktopInk() {
    desktopInkRemoved = desktopInkClip.removeMovieClip();
    desktopInkClip = false;
    desktopInkViewport = false;
}
