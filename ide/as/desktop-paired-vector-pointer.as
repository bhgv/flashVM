// Copyright (c) 2026 bhgv
// EN: Map ordinary pointer events using the published drawing and parent geometry.
// RU: Преобразовать события указателя по опубликованной геометрии рисунка и родителя.


// EN: Use the same fit, parent and table transform as the committed canvas.
// Return a packed Dis Draw offset for a point on its 1000-by-600 logical page.
// RU: Использовать масштаб, родителя и преобразование стола показанного холста.
// Вернуть смещение Dis Draw для точки на логической странице 1000 на 600.
function placeDesktopVectorPointer() {
    desktopVectorPointerSlot = ("0x" + desktopPointerId) * 1 - 1;
    desktopVectorPointerAt = desktopVectorPointerSlot * 25;
    desktopVectorPointerParent = ("0x" + desktopFrontSources.substr(desktopVectorPointerAt, 1)) * 1;
    desktopVectorPointerWidth = ("0x" + desktopFrontSources.substr(desktopVectorPointerAt + 9, 8)) * 1;
    desktopVectorPointerHeight = ("0x" + desktopFrontSources.substr(desktopVectorPointerAt + 17, 8)) * 1;
    desktopVectorOrigin = desktopFrontSourcePositions.substr(desktopVectorPointerSlot * 16, 16);
    desktopReadOrigin = _root.readDesktopVectorOrigin();
    desktopVectorPaneX = desktopVectorOriginX;
    desktopVectorPaneY = desktopVectorOriginY;
    if (Math.max(0, desktopVectorPointerParent - 4)) {
        desktopVectorGroup = desktopVectorPointerParent - 5;
        desktopVectorOrigin = desktopFrontGroupPositions.substr(desktopVectorGroup * 16, 16);
        desktopReadOrigin = _root.readDesktopVectorOrigin();
        desktopVectorPaneX = desktopVectorPaneX + desktopVectorOriginX;
        desktopVectorPaneY = desktopVectorPaneY + desktopVectorOriginY;
        desktopVectorPointerParent = desktopFrontGroups.substr(desktopVectorGroup * 9, 1) - 1;
    }
    desktopVectorMouseX = desktopPointerX;
    desktopVectorMouseY = desktopPointerY;
    if (!(desktopVectorPointerParent - 1)) {
        desktopVectorMouseX = (desktopVectorMouseX - desktopFrontTableX) / desktopFrontTableScale;
        desktopVectorMouseY = (desktopVectorMouseY - desktopFrontTableY) / desktopFrontTableScale;
    }
    desktopVectorPointerScale = Math.min((desktopVectorPointerWidth - 16) / 1000,
        (desktopVectorPointerHeight - 32) / 600);
    desktopVectorPointerScale = Math.max(1 / 1000, desktopVectorPointerScale);
    desktopVectorMouseX = (desktopVectorMouseX - desktopVectorPaneX
        - (desktopVectorPointerWidth - 1000 * desktopVectorPointerScale) / 2) / desktopVectorPointerScale;
    desktopVectorMouseY = (desktopVectorMouseY - desktopVectorPaneY - 28) / desktopVectorPointerScale;
    if (Math.max(0, 0 - desktopVectorMouseX) + Math.max(0, desktopVectorMouseX - 1000)
        + Math.max(0, 0 - desktopVectorMouseY) + Math.max(0, desktopVectorMouseY - 600)) {
        desktopPointerId = "00000000";
    } else {
        desktopPointerOffset = Math.round(desktopVectorMouseX) + Math.round(desktopVectorMouseY) * 1001;
    }
}
