// Copyright (c) 2026 bhgv
// EN: Resolve a mirror's document against the matching staged source scene.
// RU: Найти документ зеркала в том же подготовленном снимке сцены.


// EN: Use the source's actual layer or MDI owner and read its declared local extent.
// RU: Использовать слой или родителя MDI исходного клипа и прочитать его локальный размер.
function resolveDesktopMirror() {
    desktopMirrorSource = false;
    desktopSourceIds = ",1,2,3,4,5,6,7,8,";
    if (desktopSourceIds.indexOf("," + desktopMirror + ",") + 1) {
        desktopSourceOffset = (desktopMirror - 1) * 25;
        desktopSourceDepth = ("0x" + desktopSources.substr(desktopSourceOffset + 1, 8)) * 1;
        if (desktopSourceDepth) {
            desktopTargetParent = desktopParent;
            desktopParent = ("0x" + desktopSources.substr(desktopSourceOffset, 1)) * 1;
            desktopSourcePlaced = _root.placeDesktopItem();
            desktopParent = desktopTargetParent;
            if (desktopOwner) {
                desktopMirrorSource = desktopOwner.getInstanceAtDepth(desktopSourceDepth);
                desktopSourceWidth = ("0x" + desktopSources.substr(desktopSourceOffset + 9, 8)) * 1;
                desktopSourceHeight = ("0x" + desktopSources.substr(desktopSourceOffset + 17, 8)) * 1;
            }
        }
    }
}
