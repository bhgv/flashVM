# Selection presentation serialization, independent of source/history ownership.


# Build only a dirty overlay; retain the unchanged source string by reference.
# doc belongs to one pane. No callbacks, I/O or shared mutable worker state occurs.
# The source prefix is copied once per geometry identity, never per Snapshot.
presentation(doc: ref Document)
{
    if (doc.presentation == nil) {
        doc.presentation = ref Presentation(nil,-1,-2,1,"","","",0,"",nil,0);
    }
    p := doc.presentation;
    if (p.dirty && presentationreuse(doc,p)) {
        return;
    }
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
    p.plan = "";
    count := len doc.shapes;
    if (overlay != nil) {
        p.plan = editplan(doc);
        bytes: array of byte;
        if (p.plan != "" && doc.tool == Nodes) {
            # The native generic binding renderer reconstructs decorations from
            # copied values. Do not serialize thousands of redundant coordinates.
            bytes = array[32] of byte;
            putword(bytes,0,overlay.color);
            putword(bytes,8,overlay.width);
            putword(bytes,16,overlay.fill);
            putword(bytes,24,len overlay.nodes);
        } else {
            bytes = array[32+(40+16*doc.wirecubic)*len overlay.nodes] of byte;
            wireshape(bytes,0,overlay,doc.wirecubic,doc.work);
        }
        p.overlay = string bytes;
        count++;
    }
    header := array[8] of byte;
    putword(header,0,count);
    p.count = string header;
    p.selection = nil;
    p.mode = 0;
    if (doc.edit != nil) {
        p.selection = array[len doc.edit.mask] of int;
        p.selection[0:] = doc.edit.mask;
        p.mode = doc.edit.mode;
    }
    p.tool = doc.tool;
    p.selected = selected;
    p.dirty = 0;
    if (p.builds < 2147483647) {
        p.builds++;
    }
}


include "idevectoroverlaycache.b";
