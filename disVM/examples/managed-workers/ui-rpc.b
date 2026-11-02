implement ManagedUI;
include "sys.m";
include "idehost.m";

ManagedUI: module {
    init: fn(); # Requires disvm-editor, its UI main thread and an explicit font.
};


# Repeated copied UI queries cross managed scheduling boundaries. No MovieClip,
# TextField, renderer or foreign heap address enters this Dis module. The helper
# must hold its ACTIVE claimed turn in the SAME runtime domain to issue RPC.
init()
{
    sys := load Sys Sys->PATH;
    host := load Idehost Idehost->PATH;
    if (host == nil) {
        raise "fail: managed UI host unavailable";
    }
    for (i := 0; i < 64; i++) {
        rows := host->rows();
        lines := host->linenumbers();
        path := host->path();
        if (rows < 1 || (lines != 0 && lines != 1) || path != "") {
            raise "fail: managed UI copied reply";
        }
    }
    sys->print("PASS managed UI RPC: copied replies from both OS owners\n");
}
