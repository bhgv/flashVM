// Copyright (c) 2026 bhgv
// EN: Poll local input and arm terminal notifications on the FlashVM owner.
// RU: Опрос локального ввода и включение уведомлений у владельца FlashVM.

// EN: Initialize the adapters and give pending input a turn on the movie clock.
// RU: Инициализировать адаптеры и обработать ожидающий ввод на такте фильма.
function onEnterFrame() {
    if (!desktopFrontDepth) {
        desktopFrontDepth = 10;
        desktopSign = "89abcdef";
        desktopNotifications = styxTerminal.enableNotifications();
        if (hostInput) {
            desktopInputEnabled = hostInput.enable();
        }
    }
    desktopTerminalTurn = _root.onTerminalFrame();
}
