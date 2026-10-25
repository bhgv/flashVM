implement NativeIde; # Run real IDE services through the embedded native Dis host.
include "sys.m"; # Import actual output formatting from the supported Sys subset.
include "idecore.m"; # Import the document/history channel protocol.
include "ideworkspace.m"; # Import the workspace owner and build-ticket protocol.
include "main.m"; # Import the fully commented callable workspace tutorial.


# Initial native console entry; no Draw, Tk or external emulator is required.
NativeIde: module
{ # The native runner invokes this parameterless entry function.
    init: fn(); # Run the real workspace example and wait for its services to stop.
}; # Keep the bootstrap interface independent of a graphical context.


# Load production IDE modules, execute their channel-based tutorial and report completion.
# No parameters. Returns after the workspace and all document owners acknowledge shutdown.
init()
{ # This body executes as Dis instructions, not as a native replacement of IDE behavior.
    sys := load Sys Sys->PATH; # Resolve the native host's supported Sys exports.
    core := load Idecore Idecore->PATH; # Load the separately compiled document/history service.
    workspace := load Ideworkspace Ideworkspace->PATH; # Load workspace ownership on Dis.
    example := load WorkspaceExample WorkspaceExample->PATH; # Load the reusable tutorial module.
    if (sys == nil || core == nil || workspace == nil || example == nil) { # Reject failed links explicitly.
        raise "fail: native IDE module load"; # The native runner returns failure for this exception.
    } # Continue only with real linked module instances.
    if (!example->run(core, workspace)) { # Exercise actual spawn, channels, documents and build tickets.
        raise "fail: native IDE workspace example"; # Never print PASS after a failed service exchange.
    } # The tutorial has consumed every acknowledgement and stopped its private owners.
    sys->print("PASS native Dis IDE workspace: documents, build snapshot, channels and shutdown\n"); # State the tested scope.
} # No graphical editor is claimed by this console entry.
