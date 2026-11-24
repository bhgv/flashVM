// Copyright (c) 2026 bhgv
// EN: Resolve each item against the staged layer or its actual MDI root.
// RU: Определить слой или настоящий корень MDI для элемента снимка.


// EN: Use the parent code in ItemView; a child born inside an MDI root keeps local geometry.
// RU: Использовать код родителя ItemView. Дочерняя панель MDI сохраняет локальные координаты.
function placeDesktopItem() {
    desktopOwner = false;
    desktopLayerCodes = ",0,1,2,3,4,";
    if (desktopLayerCodes.indexOf("," + desktopParent + ",") + 1) {
        desktopOwner = desktopPending.getInstanceAtDepth(desktopParent + 1);
        if (!(desktopId - 63)) {
            desktopOwner = desktopOverlayLayer;
        }
    } else {
        desktopGroupParents = ",5,6,7,8,9,10,11,12,";
        if (desktopGroupParents.indexOf("," + desktopParent + ",") + 1) {
            desktopSlot = (desktopParent - 5) * 9;
            desktopOwnerLayer = ("0x" + desktopStagedGroups.substr(desktopSlot, 1)) * 1;
            desktopOwnerDepth = ("0x" + desktopStagedGroups.substr(desktopSlot + 1, 8)) * 1;
            if (desktopOwnerDepth) {
                desktopOwner = desktopPending.getInstanceAtDepth(desktopOwnerLayer);
                desktopOwner = desktopOwner.getInstanceAtDepth(desktopOwnerDepth);
            }
        }
    }
}
