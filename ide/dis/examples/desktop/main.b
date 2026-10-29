implement DesktopExample; # Run this tutorial as an ordinary Dis entry module.
include "desktop.m"; # Use the typed application entry, not a native GUI API.

DesktopExample: module
{
    init: fn(); # Enter the real MovieClip desktop on this interpreter/JIT.
};


# Load and retain the actual Limbo application until its acknowledged shutdown.
# No parameters. Its own document/pane processes communicate through channels;
# FlashVM owns every MovieClip and TextField. No external Inferno/CLI is spawned.
init()
{
    desktop := load DesktopIDE DesktopIDE->PATH; # Resolve the installed bytecode module.
    if (desktop == nil) { # Never substitute a static mockup on an unavailable host.
        raise "fail: MovieClip desktop application unavailable"; # Report a real load failure.
    } # Continue only with a matching typed interface.
    desktop->init(); # Owns actual independent editors, log, help and desktop channels.
} # The call returns after pane/layout owners retire and native ports detach.
