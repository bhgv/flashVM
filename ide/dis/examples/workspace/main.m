# Include idecore.m and ideworkspace.m before this small educational module header.
WorkspaceExample: module
{ # Begin the portable callable example interface.
    PATH: con "workspace-example.dis";                     # Host-resolved resource; no filesystem assumption.
    run: fn(core: Idecore, workspace: Ideworkspace): int;  # Borrow loaded modules; return one on balanced success.
}; # End the interface without importing Sys, Draw or a graphical toolkit.
