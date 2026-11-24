// Copyright (c) 2026 bhgv
// EN: Pull one input transition after Dis acknowledges the previous scene.
// RU: Снять одно событие ввода после подтверждения предыдущей сцены Dis.


// EN: Stage a single transition for the existing credit-based sender.
// RU: Подготовить одно событие к передаче существующим отправителем с кредитом.
function pollDesktopInput() {
    desktopInputKind = hostInput.take();
    if (desktopInputKind * (desktopInputKind - 2) * !(!desktopInkClip)) {
        desktopInkDiscarded = _root.discardDesktopInk();
        desktopInkPresented = _root.publishDesktopInk();
    }
    if (!(desktopInputKind - 1)) {
        desktopWord = hostInput.get("code");
        desktopEncoded = _root.encodeDesktopWord();
        desktopKeyWord = desktopHexWord;
        desktopWord = hostInput.get("phase");
        desktopEncoded = _root.encodeDesktopWord();
        desktopOutgoing = "fvd1:0000000400000000" + desktopKeyWord + desktopHexWord + "0000000000000000";
    }
    if (!(desktopInputKind - 2)) {
        desktopPointerPhase = hostInput.get("phase");
        desktopPointerCode = hostInput.get("code");
        if (!(desktopPointerCode - 2)) {
            desktopPointerCode = 7;
        }
        if (!(desktopPointerPhase - 1)) {
            desktopPointerPhase = 2;
        } else {
            if (!(desktopPointerPhase - 2)) {
                desktopPointerPhase = 1;
            }
        }
        desktopPointerSent = _root.sendDesktopPointer();
    }
    if (!(desktopInputKind - 3)) {
        desktopPointerCode = 5;
        desktopWheel = hostInput.get("wheel");
        desktopWheelMagnitude = Math.abs(desktopWheel);
        if (!(desktopWheel - desktopWheelMagnitude)) {
            desktopPointerCode = 4;
        }
        desktopPointerPhase = 1;
        desktopWheelControl = hostInput.get("control");
        if (desktopWheelControl) {
            desktopPointerCode = desktopPointerCode + 4;
        } else {
            desktopWheelSteps = Math.floor(desktopWheelMagnitude / 3);
            desktopWheelSteps = Math.min(16, desktopWheelSteps);
            desktopWheelMinimum = Math.max(2, desktopWheelSteps);
            if (!(desktopWheelMinimum - desktopWheelSteps)) {
                desktopPointerPhase = 16 + desktopWheelSteps;
            }
        }
        desktopPointerSent = _root.sendDesktopPointer();
    }
    if (!(desktopInputKind - 4)) {
        desktopTyped = hostInput.get("text");
        if (desktopTyped) {
            desktopOutgoing = "fvd1:000000050000000000000000000000000000000000000000" + desktopTyped;
        }
    }
    if (!(desktopInputKind - 5)) {
        desktopWord = hostInput.get("phase");
        desktopEncoded = _root.encodeDesktopWord();
        desktopOutgoing = "fvd1:000000060000000000000000" + desktopHexWord + "0000000000000000";
    }
    if (!(desktopInputKind - 6)) {
        desktopOutgoing = "fvd1:000000010000000100010003000000000000000000000000";
    }
    if (!(desktopInputKind - 7)) {
        desktopResizeWidth = hostInput.get("x");
        desktopResizeHeight = hostInput.get("y");
        if (Math.abs(desktopResizeWidth - desktopFrontWidth) + Math.abs(desktopResizeHeight - desktopFrontHeight)) {
            desktopResizePending = true;
            desktopResizeSent = _root.sendDesktopResize();
        }
    }
}
