implement IdecompleteTest;
include "sys.m";
include "idecomplete.m";

sys: Sys;          # Reference-test diagnostics only.
service: Idecomplete; # Actual compiled owner module.


# Emulator init contract used only by this isolated reference test.
IdecompleteTest: module
{
    init: fn(); # No graphical environment or boot shell.
};


# Stop the selected reference emulator after a visible result.
# No parameters. Returns never; a missing control raises a diagnostic exception.
halt()
{
    control := sys->open("/dev/sysctl", Sys->OWRITE);
    if (control == nil) {
        raise "fail: cannot halt reference completion test";
    }
    data := array of byte "halt";
    sys->write(control, data, len data);
    raise "fail: reference halt returned";
}


# Surface a failed completion invariant explicitly.
# message is borrowed diagnostic text. Returns never.
fail(message: string)
{
    sys->print("FAIL idecomplete %s\n", message);
    halt();
}


# Exchange one real typed channel request, with a private synchronous reply rendezvous.
# commands/operation/generation/prefix/limit/items are borrowed or copied values. Returns the owner reply.
exchange(commands: chan of Idecomplete->Request, operation, generation: int,
         prefix: string, limit: int, items: array of ref Idecomplete->Item): Idecomplete->Reply
{
    response := chan of Idecomplete->Reply;
    commands <-= Idecomplete->Request(1, operation, generation, prefix, limit, items, response);
    return <-response;
}


# Check publication ordering, generation conflicts, Unicode lookup and both alias boundaries.
# No parameters. Prints PASS and halts after the real owner acknowledges Shutdown.
init()
{
    sys = load Sys Sys->PATH;
    service = load Idecomplete Idecomplete->PATH;
    if (service == nil) {
        fail("module loading");
    }
    commands := service->start();
    items := array[4] of ref Idecomplete->Item;
    items[0] = ref Idecomplete->Item("print", 2, "print(string)", "formatted output", nil);
    items[1] = ref Idecomplete->Item("process", 1, "process()", "owner service", nil);
    items[2] = ref Idecomplete->Item("printf", 2, "printf(string)", "formatted output", nil);
    items[3] = ref Idecomplete->Item("привет", 2, "привет()", "Unicode declaration", nil);
    parameters := array[1] of ref Idecomplete->Parameter;
    parameters[0] = ref Idecomplete->Parameter("format", "string", 0, "Output format.");
    items[0].parameters = parameters;
    reply := exchange(commands, service->Publish, 1, "", 0, items);
    if (reply.status != service->Ok || reply.generation != 1) {
        fail("atomic unsorted publication");
    }
    items[0].name = "corrupted provider alias";
    parameters[0].name = "corrupted provider parameter";
    reply = exchange(commands, service->Query, 1, "pri", 64, nil);
    if (reply.status != service->Ok || len reply.items != 2
        || reply.items[0].name != "print" || reply.items[1].name != "printf"
        || reply.items[0].parameters[0].name != "format") {
        fail("prefix ordering or provider alias isolation");
    }
    reply.items[0].name = "corrupted result alias";
    reply.items[0].parameters[0].name = "corrupted result parameter";
    reply = exchange(commands, service->Query, 1, "pri", 1, nil);
    if (reply.status != service->Ok || len reply.items != 1 || reply.items[0].name != "print"
        || reply.items[0].parameters[0].name != "format") {
        fail("result alias isolation or result bound");
    }
    reply = exchange(commands, service->Query, 1, "при", 64, nil);
    if (reply.status != service->Ok || len reply.items != 1 || reply.items[0].name != "привет") {
        fail("Unicode prefix lookup");
    }
    reply = exchange(commands, service->Query, 0, "", 64, nil);
    if (reply.status != service->Conflict || reply.generation != 1) {
        fail("stale query generation");
    }
    duplicate := array[2] of ref Idecomplete->Item;
    duplicate[0] = ref Idecomplete->Item("same", 1, "()", "first", nil);
    duplicate[1] = ref Idecomplete->Item("same", 1, "()", "second", nil);
    reply = exchange(commands, service->Publish, 2, "", 0, duplicate);
    if (reply.status != service->Invalid || reply.generation != 1) {
        fail("duplicate batch replaced admitted generation");
    }
    reply = exchange(commands, service->Query, 1, "pri", 64, nil);
    if (reply.status != service->Ok || len reply.items != 2) {
        fail("failed publication damaged the index");
    }
    reply = exchange(commands, service->Shutdown, 0, "", 0, nil);
    if (reply.status != service->Ok) {
        fail("shutdown acknowledgement");
    }
    sys->print("PASS idecomplete publication, prefix search, Unicode, generations and alias isolation\n");
    halt();
}
