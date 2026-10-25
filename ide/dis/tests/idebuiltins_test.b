implement IdebuiltinsTest;
include "sys.m";
include "idecomplete.m";
include "idebuiltins.m";

sys: Sys;                # Reference test diagnostics, never a provider dependency.
index: Idecomplete;      # Real typed completion owner module.
builtins: Idebuiltins;   # Real AS and Limbo declarations/structured parameter help.


# Isolated emulator test entry, without shell or graphical initialization.
IdebuiltinsTest: module
{
    init: fn(); # The reference harness loads this directly.
};


# Halt only the caller-selected reference test runtime.
# No parameters. Returns never; missing control raises a diagnostic exception.
halt()
{
    control := sys->open("/dev/sysctl", Sys->OWRITE);
    if (control == nil) {
        raise "fail: cannot halt builtin catalog test";
    }
    data := array of byte "halt";
    sys->write(control, data, len data);
    raise "fail: builtin test halt returned";
}


# Report a concrete catalog/help failure before halting the test runtime.
# message is borrowed. Returns never.
fail(message: string)
{
    sys->print("FAIL idebuiltins %s\n", message);
    halt();
}


# Use the actual owner request/reply channels; all calls are serialized by this test caller.
# commands/op/gen/prefix/items are borrowed values. Returns the copied owner response.
exchange(commands: chan of Idecomplete->Request, op, gen: int, prefix: string,
         items: array of ref Idecomplete->Item): Idecomplete->Reply
{
    response := chan of Idecomplete->Reply;
    commands <-= Idecomplete->Request(1, op, gen, prefix, 64, items, response);
    return <-response;
}


# Check both languages, objects/methods/operators, overloads, optional/rest args and help isolation.
# No parameters. Prints PASS and halts after both index owners acknowledge shutdown.
init()
{
    sys = load Sys Sys->PATH;
    index = load Idecomplete Idecomplete->PATH;
    builtins = load Idebuiltins Idebuiltins->PATH;
    if (index == nil || builtins == nil) {
        fail("module loading");
    }
    asindex := index->start();
    limboindex := index->start();
    asitems := builtins->actionscript();
    limboitems := builtins->limbo();
    reply := exchange(asindex, index->Publish, 1, "", asitems);
    if (reply.status != index->Ok || len asitems < 100) {
        fail("AS catalog publication");
    }
    reply = exchange(limboindex, index->Publish, 1, "", limboitems);
    if (reply.status != index->Ok || len limboitems < 50) {
        fail("Limbo catalog publication");
    }
    reply = exchange(asindex, index->Query, 1, "MovieClip.goto", nil);
    if (len reply.items != 2 || reply.items[0].name != "MovieClip.gotoAndPlay") {
        fail("AS qualified method completion");
    }
    tips := builtins->help(reply.items, "MovieClip.gotoAndPlay", 0);
    if (len tips != 1 || tips[0].active != 0 || tips[0].parameter == nil
        || tips[0].parameter.name != "frame" || tips[0].parameter.summary == "") {
        fail("AS method parameter short help");
    }
    tips[0].parameter.name = "mutated help result";
    tips = builtins->help(reply.items, "MovieClip.gotoAndPlay", 0);
    if (tips[0].parameter.name != "frame") {
        fail("call-tip result aliased candidate parameter");
    }
    reply = exchange(asindex, index->Query, 1, "MovieClip.hitTest", nil);
    tips = builtins->help(reply.items, "MovieClip.hitTest", 2);
    optional := 0;
    for (i := 0; i < len tips; i++) {
        if (tips[i].active == 2 && tips[i].parameter != nil
            && tips[i].parameter.name == "shape" && tips[i].parameter.optional) {
            optional++;
        }
    }
    if (len tips != 2 || optional != 1) {
        fail("overload-specific optional argument help");
    }
    reply = exchange(asindex, index->Query, 1, "Math.max", nil);
    tips = builtins->help(reply.items, "Math.max", 7);
    if (len tips != 1 || tips[0].active != 0 || tips[0].parameter.name != "...values") {
        fail("variadic active argument help");
    }
    reply = exchange(asindex, index->Query, 1, "+", nil);
    if (len reply.items < 2 || reply.items[0].kind != builtins->Operator) {
        fail("AS operator completion");
    }
    reply = exchange(asindex, index->Query, 1, "TextField.setTextFormat", nil);
    if (len reply.items != 1 || len reply.items[0].parameters != 1) {
        fail("catalog advertised an unsupported mixed-format setter overload");
    }
    reply = exchange(limboindex, index->Query, 1, "Sys.open", nil);
    tips = builtins->help(reply.items, "Sys.open", 1);
    if (len tips != 1 || tips[0].parameter.name != "mode" || tips[0].parameter.type_name != "int") {
        fail("Limbo Sys parameter help");
    }
    reply = exchange(limboindex, index->Query, 1, "chan", nil);
    if (len reply.items != 1 || reply.items[0].kind != builtins->Keyword) {
        fail("Limbo keyword completion");
    }
    reply = exchange(asindex, index->Shutdown, 0, "", nil);
    if (reply.status != index->Ok) {
        fail("AS catalog owner shutdown");
    }
    reply = exchange(limboindex, index->Shutdown, 0, "", nil);
    if (reply.status != index->Ok) {
        fail("Limbo catalog owner shutdown");
    }
    sys->print("PASS idebuiltins %d AS and %d Limbo entries; overloads, parameters and help\n", len asitems, len limboitems);
    halt();
}
