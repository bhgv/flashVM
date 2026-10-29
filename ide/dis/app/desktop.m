# Typed entry into the MovieClip desktop; no native GUI pointer crosses this boundary.
DesktopIDE: module
{
    PATH: con "desktop-ide.dis"; # Resolve the matched application beside its dependencies.
    init: fn(); # Block until all pane/layout owners acknowledge shutdown.
};
