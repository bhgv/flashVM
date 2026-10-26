# Stable application-to-controller boundary; both modules execute on our DisVM.
GraphicalEditor: module
{
    PATH: con "graphical-editor.dis"; # Resolve beside the installed application modules.
    init: fn(); # Own typed document/UI channels until acknowledged shutdown.
};
