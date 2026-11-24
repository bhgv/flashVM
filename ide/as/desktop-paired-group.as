// Copyright (c) 2026 bhgv
// EN: Register the current MDI root's scene-local placement.
// RU: Записать размещение текущего корня MDI в снимке.


// EN: Record the layer and paint depth for child descriptors referencing this root.
// RU: Сохранить слой и глубину для дочерних записей, ссылающихся на этот корень.
function registerDesktopGroup() {
    desktopGroupIds = "|13|14|15|16|17|18|19|28|";
    desktopGroupIndex = desktopGroupIds.indexOf("|" + desktopId + "|");
    desktopGroup = desktopGroupIndex + 1;
    if (desktopGroup) {
        desktopGroupOffset = desktopGroupIndex * 3;
        desktopWord = desktopSeen;
        desktopGroupEncoded = _root.encodeDesktopWord();
        desktopStagedGroups = desktopStagedGroups.substr(0, desktopGroupOffset) + (desktopParent + 1)
            + desktopHexWord + desktopStagedGroups.substr(desktopGroupOffset + 9);
        desktopGroupPositionOffset = desktopGroupIndex / 3 * 16;
        desktopGroupPositions = desktopGroupPositions.substr(0, desktopGroupPositionOffset)
            + desktopWire.substr(61, 16) + desktopGroupPositions.substr(desktopGroupPositionOffset + 16);
    }
}
