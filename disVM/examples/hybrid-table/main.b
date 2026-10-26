# Execute the hosted native scalar tab API on the real Dis interpreter/JIT.
# The extended tab syntax is tested separately by our Coco/R frontend.
implement Tablecheck;
include "sys.m";
include "bigtable.m";


Tablecheck: module
{
    init: fn(); # Test entry without a Draw or operating-system dependency.
};


# Verify typed storage, sequence holes, copied slices and snapshots after owner retirement.
# Native references are opaque and GC-managed; the module never exposes a C pointer.
init()
{
    s := load Sys Sys->PATH;
    tables := load Bigtable Bigtable->PATH;
    if (s == nil || tables == nil) {
        raise "fail: hybrid tab modules";
    }
    tab := tables->create();
    maximum := big 9223372036854775807;
    if (tables->setindex(tab, big 1, maximum) != tables->Ok
        || tables->length(tab) != big 0
        || tables->setindex(tab, big 0, big 0) != tables->Ok
        || tables->length(tab) != big 2
        || tables->setindex(tab, maximum, big 80) != tables->Ok
        || tables->settext(tab, "ключ😀", big 30) != tables->Ok
        || tables->settext(tab, "1", big 40) != tables->Ok
        || tables->settext(tab, "a\0b", big 50) != tables->Ok) {
        raise "fail: hybrid tab insertion and holes";
    }
    (status, value) := tables->getindex(tab, big 1);
    if (status != tables->Ok || value != maximum) {
        raise "fail: exact big tab value";
    }
    (status, value) = tables->gettext(tab, "ключ😀");
    if (status != tables->Ok || value != big 30) {
        raise "fail: Unicode dictionary key";
    }
    (status, value) = tables->gettext(tab, "a\0b");
    if (status != tables->Ok || value != big 50) {
        raise "fail: embedded zero dictionary key";
    }
    (status, value) = tables->getindex(tab, big 7);
    if (status != tables->Missing || tables->count(tab) != big 6) {
        raise "fail: missing differs from zero or pair count";
    }
    copy := tables->slice(tab, big 1, big 2);
    if (tables->length(copy) != big 1 || tables->count(copy) != big 1
        || tables->setindex(copy, big 0, big 9) != tables->Ok) {
        raise "fail: slice shape";
    }
    (status, value) = tables->getindex(tab, big 1);
    if (status != tables->Ok || value != maximum) {
        raise "fail: slice aliases source slots";
    }
    keys := tables->keysof(tab);
    if (tables->keylen(keys) != big 6) {
        raise "fail: complete key snapshot";
    }
    (kind, number, text) := tables->keyat(keys, big 2);
    if (kind != tables->Index || number != maximum) {
        raise "fail: sorted sparse numeric key";
    }
    if (tables->delindex(tab, big 0) != tables->Ok || tables->length(tab) != big 0
        || tables->deltext(tab, "ключ😀") != tables->Ok) {
        raise "fail: tab deletion";
    }
    # Dropping source owners does not invalidate the copied key sequence.
    tab = nil;
    copy = nil;
    (kind, number, text) = tables->keyat(keys, big 3);
    if (kind != tables->Text || text != "ключ😀") {
        raise "fail: keys retain source storage";
    }
    # A snapshot key can populate another owner after the source was retired.
    # The embedded zero byte is carried by the copied key, without string conversion.
    tab = tables->create();
    tables->keycheck(keys, big 5);
    if (tables->setkey(tab, keys, big 5, big 66) != tables->Ok) {
        raise "fail: snapshot key write";
    }
    (status, value) = tables->getkey(tab, keys, big 5);
    if (status != tables->Ok || value != big 66) {
        raise "fail: snapshot key read";
    }
    (status, value) = tables->gettext(tab, "a\0b");
    if (status != tables->Ok || value != big 66) {
        raise "fail: snapshot key lost binary text identity";
    }
    # Reflection reports the key's data kind rather than the internal capability layout.
    # Text copying preserves the embedded zero; numeric formatting never passes through real.
    if (tables->keytype(keys, big 1) != "big"
        || tables->keytype(keys, big 5) != "string"
        || tables->typename(tables->TableKind) != "table of big"
        || tables->keybig(keys, big 4) != big 1
        || tables->keytext(keys, big 5) != "a\0b"
        || tables->bigfromtext(tables->textfrombig(-big 9223372036854775808)) != -big 9223372036854775808) {
        raise "fail: reflected types and exact copied conversions";
    }
    # Exercise repeated native owner replacement; RC/GC invokes the C storage finalizers.
    for (i := 0; i < 2000; i++) {
        tab = tables->create();
        if (tables->settext(tab, "temporary", big i) != tables->Ok) {
            raise "fail: replacement tab allocation";
        }
    }
    s->print("PASS native hybrid tables: exact big Unicode holes slices key snapshots lifecycle\n");
}
