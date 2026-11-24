// Copyright (c) 2026 bhgv
// EN: Map a point inside the fixed paper to the document camera.
// RU: Преобразовать точку внутри страницы по камере документа.


// EN: Apply the published inverse camera and encode a point on the logical page.
// RU: Применить обратное преобразование камеры и закодировать точку страницы.
function unprojectDesktopVectorPointer() {
    desktopPointerZoom = ("0x" + desktopPointerCamera.substr(0, 8)) * 1;
    desktopPointerCenterX = ("0x" + desktopPointerCamera.substr(8, 8)) * 1;
    desktopPointerCenterY = ("0x" + desktopPointerCamera.substr(16, 8)) * 1;
    desktopPointerScale = desktopVectorPointerScale * desktopPointerZoom / 100;
    desktopVectorDocX = desktopPointerCenterX + (desktopVectorMouseX - 500 * desktopVectorPointerScale) / desktopPointerScale;
    desktopVectorDocY = desktopPointerCenterY + (desktopVectorMouseY - 300 * desktopVectorPointerScale) / desktopPointerScale;
    desktopVectorPointerInside = !(Math.max(0, 0 - desktopVectorDocX) + Math.max(0, desktopVectorDocX - 1000)
        + Math.max(0, 0 - desktopVectorDocY) + Math.max(0, desktopVectorDocY - 600));
    if (desktopVectorPointerInside) {
        desktopPointerOffset = Math.round(desktopVectorDocX) + Math.round(desktopVectorDocY) * 1001;
    }
}
