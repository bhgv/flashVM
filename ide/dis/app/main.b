implement FlashIDE;
include "ide.m"; # Load the existing channel-owned controller through a typed interface.


FlashIDE: module
{
    init: fn(); # Parameterless entry for our native DisVM/FlashVM host.
};


# Start the actual Dis controller, keeping the loaded module rooted on this call stack.
# The controller owns document/input processes and typed UI channels; this launcher
# performs no direct UI access. Missing/incompatible modules fail explicitly.
# Blocks until clean or explicitly forced application closure; no external emu runs.
init()
{
    controller := load GraphicalEditor GraphicalEditor->PATH;
    if (controller == nil) {
        raise "fail: cannot load IDE graphical controller";
    }
    controller->init(); # Execute editor policy on this same interpreter/JIT and scheduler.
}
