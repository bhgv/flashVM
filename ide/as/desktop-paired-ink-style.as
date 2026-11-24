// Copyright (c) 2026 bhgv
// EN: Store cycle-local ink values beside their staged pane camera.
// RU: Сохранить значения штриха рядом с камерой панели в готовящемся снимке.


// EN: Record revision, colour, width and tool for a complete vector stream.
// RU: Записать ревизию, цвет, толщину и инструмент завершённого потока.
function registerDesktopInkStyle() {
    desktopStagedInkStyles = desktopStagedInkStyles.substr(0, desktopVectorAt)
        + desktopVectorInkStyle + desktopStagedInkStyles.substr(desktopVectorAt + 25);
}
