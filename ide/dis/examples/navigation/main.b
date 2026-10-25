# Runnable developer example; Sys is used only by this host bootstrap.
implement Navigation;  # Name this example's module.
include "sys.m";       # Explicit external reporting interface.
sys: Sys;              # Loaded once by init below.
include "idecore.m";   # Share the exact 0.3.x channel ABI.
core: Idecore;         # Product module loaded by the bootstrap.


# A reference emulator invokes init; production clients call Idecore.start instead.
Navigation: module  # This is not an IDE UI implementation.
{
    init: fn();  # No Draw/Tk context or shell argument.
};

# Assertion count belongs only to the example, never to the document service.
checks := 0;  # Printed after acknowledged shutdown.


# Halt only this example's private emulator; never use this inside product code.
halt()
{
    control := sys->open("/dev/sysctl", Sys->OWRITE);  # Test root owns this control device.
    if(control == nil)                                 # A missing device is a visible failure.
    {
        raise "fail: missing example halt control";
    }                                     # Do not silently hang the host.
    data := array of byte "halt";         # Encode the explicit control request.
    sys->write(control, data, len data);  # Stop this isolated reference emulator.
    raise "fail: halt returned";          # Reaching here is an error.
}


# Make errors visible even if the emulator reports a root-process trap as exit zero.
require(ok: int, message: string)
{
    if(!ok)
    {                                      # Check every assumption used for an edit.
        sys->print("FAIL %s\n", message);  # The host runner rejects this marker.
        halt();                            # End only the example's emulator.
    }
    checks++;  # Count only checks that actually passed.
}


# Keep the caller-owned reply channel alive until the service answers exactly once.
exchange(commands: chan of core->Request, operation, expected, first, last: int,
text: string): core->Reply
{
    reply := chan of core->Reply;  # One rendezvous destination per request.
    commands <-= core->Request(7, operation, expected, first, last, text, "", reply);
    result := <-reply;                          # Receive before issuing dependent work.
    require(result.id == 7, "correlation ID");  # This caller serializes its requests.
    return result;                              # Retain a value snapshot, not mutable state.
}


# Demonstrate navigation, exact search, an edit and rejection of stale coordinates.
init()
{
    sys = load Sys Sys->PATH;                  # Bootstrap-only host module.
    core = load Idecore Idecore->PATH;         # Resolve the installed product resource.
    require(core != nil, "load idecore.dis");  # Refuse an absent/incompatible product.
    commands := core->start();                 # Spawn the sole document owner.
    opened := exchange(commands, core->Open, 0, 0, 0, "A中\nxyz\n");
    require(opened.status == core->Ok, "open");  # The initial clean revision is now 1.
    located := exchange(commands, core->Locate, opened.revision, 4, 0, "");
    require(located.status == core->Ok && located.line == 2 && located.column == 1,
    "Unicode character location");  # 中 occupies one Limbo character.
    converted := exchange(commands, core->Offset, opened.revision, 2, 1, "");
    require(converted.status == core->Ok && converted.offset == 4, "inverse location");
    found := exchange(commands, core->Find, opened.revision, 0, len opened.text, "yz");
    require(found.status == core->Ok && found.offset == 4, "exact bounded match");
    sys->print("match offset=%d line=%d column=%d\n", found.offset, found.line, found.column);
    edited := exchange(commands, core->Replace, found.revision, found.offset,
    found.offset + len "yz", "YZ");  # Use the checked revision and length.
    require(edited.status == core->Ok && edited.revision == 2 && edited.saved == 1,
    "edit keeps dirty state");  # The example has not written any file.
    stale := exchange(commands, core->Find, found.revision, 0, len found.text, "YZ");
    require(stale.status == core->Conflict && stale.offset == -1, "stale search");
    require(opened.text == "A中\nxyz\n", "retained snapshot");  # Older text is still a value.
    stopped := exchange(commands, core->Shutdown, edited.revision, 0, 0, "");
    require(stopped.status == core->Ok, "shutdown");            # Never send to commands after this reply.
    sys->print("PASS idecore navigation %d checks\n", checks);  # Host acceptance marker.
    halt();                                                     # No background service remains to use.
}
