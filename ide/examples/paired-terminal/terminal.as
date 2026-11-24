// Copyright (c) 2026 bhgv
// EN: Return one portable view frame through the paired player's typed terminal.
// RU: Вернуть кадр представления через типизированный терминал парного плеера.

// The Dis controller publishes the frame before starting this timeline.
// Контроллер Dis передаёт кадр до запуска шкалы времени.
terminalFrame = styxTerminal.tryReceiveFrame();
// The fvd1 header is 53 characters. The demo payload has one prefix character and an x offset.
// Заголовок fvd1 занимает 53 символа. Данные примера содержат префикс из одного символа и смещение x.
terminalX = terminalFrame.substr(54) * 1;
_root._x = terminalX;
terminalQueued = styxTerminal.trySendFrame(terminalFrame + " AS2");
