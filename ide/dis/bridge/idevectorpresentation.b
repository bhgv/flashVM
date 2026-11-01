# Selection presentation serialization, independent of source/history ownership.


# Build only a dirty overlay; retain the unchanged source string by reference.
# doc belongs to one pane. No callbacks, I/O or shared mutable worker state occurs.
# The source prefix is copied once per geometry identity, never per Snapshot.
presentation(doc: ref Document)
{
    if (doc.presentation == nil) {
        doc.presentation = ref Presentation(nil,-1,-2,1,"","","",0);
    }
    p := doc.presentation;
    if (p.base != doc.shapes) {
        p.base = doc.shapes;
        p.paths = doc.wirebody[8:];
        p.dirty = 1;
    }
    selected := -1;
    if (doc.edit != nil) {
        selected = doc.edit.selected;
    }
    if (p.tool != doc.tool || p.selected != selected) {
        p.dirty = 1;
    }
    if (!p.dirty) {
        return;
    }
    overlay := editoverlay(doc);
    p.overlay = "";
    count := len doc.shapes;
    if (overlay != nil) {
        bytes := array[32+40*len overlay.nodes] of byte;
        wireshape(bytes,0,overlay);
        p.overlay = string bytes;
        count++;
    }
    header := array[8] of byte;
    putword(header,0,count);
    p.count = string header;
    p.tool = doc.tool;
    p.selected = selected;
    p.dirty = 0;
    if (p.builds < 2147483647) {
        p.builds++;
    }
}
