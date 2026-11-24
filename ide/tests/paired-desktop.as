// Copyright (c) 2026 bhgv
// EN: Verify the desktop scene, Unicode edit and Save As on the stock paired player.
// RU: Проверить сцену desktop, правку Unicode и сохранение через парный плеер.

// EN: Consume one record or retry one pending event on each root frame.
// RU: За кадр принять одну запись или повторить одну ожидающую отправку.
function onEnterFrame() {
    if (!desktopStarted) {
        desktopStarted = true;
        desktopPhase = 0;
    }
    if (desktopOutgoing) {
        desktopAccepted = styxTerminal.trySendFrame(desktopOutgoing);
        if (desktopAccepted) {
            desktopOutgoing = "";
        }
    } else {
        desktopWire = styxTerminal.tryReceiveFrame();
        if (desktopWire) {
            // View records for BeginView (65569), ItemView (65570), EndView (65571).
            // Записи View для BeginView (65569), ItemView (65570), EndView (65571).
            if (!desktopWire.indexOf("fvd1:000000070000000000010021")) {
                desktopEditor = false;
                desktopConsole = false;
                desktopHelp = false;
                desktopSecond = false;
                desktopChanged = false;
                desktopSaved = false;
                desktopFilenameField = false;
                desktopFilenameEntered = false;
            }
            if (!desktopWire.indexOf("fvd1:000000070000000000010022")) {
                // Two argument words begin at 37, item text after 72 metadata bytes.
                // Два слова начинаются с 37, текст следует за 72 байтами метаданных.
                desktopArguments = desktopWire.substr(37);
                desktopText = desktopWire.substr(125);
                if (!desktopArguments.indexOf("0000000100000000")) {
                    desktopEditor = desktopText.indexOf("Editor 1") + 1;
                    desktopChanged = desktopText.indexOf("Яpaired") + 1;
                    desktopSaved = desktopText.indexOf("paired-output.txt") + 1;
                }
                if (!desktopArguments.indexOf("0000000200000000")) {
                    desktopConsole = desktopText.indexOf("Build console") + 1;
                }
                if (!desktopArguments.indexOf("0000000300000000")) {
                    desktopHelp = desktopText.indexOf("Help") + 1;
                }
                if (!desktopArguments.indexOf("0000000400000000")) {
                    desktopSecond = desktopText.indexOf("Editor 4") + 1;
                }
                if (!desktopArguments.indexOf("0000003900000000")) {
                    desktopFilenameField = true;
                    desktopFilenameEntered = desktopText.indexOf("paired-output.txt") + 1;
                }
            }
            if (!desktopWire.indexOf("fvd1:000000070000000000010023")) {
                // Copy the phase before selection so one EndView advances at most once.
                // Проверять копию этапа, чтобы один EndView продвигал сценарий только один раз.
                desktopCompleted = desktopPhase;
                if (!desktopCompleted) {
                    if (desktopEditor) {
                        if (desktopConsole) {
                            if (desktopHelp) {
                                if (desktopSecond) {
                                    desktopPhase = 1;
                                    desktopOutgoing = "fvd1:000000050000000000000000000000000000000000000000Яpaired";
                                }
                            }
                        }
                    }
                }
                if (!(desktopCompleted - 1)) {
                    if (desktopChanged) {
                        // Ctrl down; the following view permits the Save shortcut.
                        // Нажать Ctrl; следующее представление допускает команду Save.
                        desktopPhase = 2;
                        desktopOutgoing = "fvd1:000000040000000000000011000000010000000000000000";
                    }
                }
                if (!(desktopCompleted - 2)) {
                    desktopPhase = 3;
                    desktopOutgoing = "fvd1:000000040000000000000053000000010000000000000000";
                }
                if (!(desktopCompleted - 3)) {
                    if (desktopFilenameField) {
                        desktopPhase = 4;
                        desktopOutgoing = "fvd1:000000040000000000000011000000000000000000000000";
                    }
                }
                if (!(desktopCompleted - 4)) {
                    if (desktopFilenameField) {
                        desktopPhase = 5;
                        desktopOutgoing = "fvd1:000000050000000000000000000000000000000000000000paired-output.txt";
                    }
                }
                if (!(desktopCompleted - 5)) {
                    if (desktopFilenameEntered) {
                        desktopPhase = 6;
                        desktopOutgoing = "fvd1:00000004000000000000000d000000010000000000000000";
                    }
                }
                if (!(desktopCompleted - 6)) {
                    if (desktopSaved) {
                        if (!desktopFilenameField) {
                            // Quit without the discard flag. Dirty shutdown would be refused.
                            // Выйти без флага отмены правок. При несохранённых правках выход отклоняется.
                            desktopPhase = 7;
                            desktopOutgoing = "fvd1:000000010000000100010003000000000000000000000000";
                        }
                    }
                }
            }
        }
    }
}
