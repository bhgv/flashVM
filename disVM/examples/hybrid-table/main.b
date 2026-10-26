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
    # Exercise repeated native owner replacement; RC/GC invokes the C storage finalizers.
    for (i := 0; i < 2000; i++) {
        tab = tables->create();
        if (tables->settext(tab, "temporary", big i) != tables->Ok) {
            raise "fail: replacement tab allocation";
        }
    }
    s->print("PASS native hybrid tables: exact big Unicode holes slices key snapshots lifecycle\n");
}
