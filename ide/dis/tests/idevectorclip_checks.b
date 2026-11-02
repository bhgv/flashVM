# Typed owner clipboard and keyboard admission through the real pane process.


# Send one clipboard command carrying its immutable SVG text and exact selection
# generation. The reply channel is private to this call; no UI/native side effects.
cliprequest(owner: chan of Idedeskpane->Request, operation: int, text: string,
            event: ref Uibridge->Message): Idedeskpane->Reply
{
    reply := chan of Idedeskpane->Reply;
    owner <-= Idedeskpane->Request(7,operation,text,event,reply);
    return <-reply;
}


# Validate fresh/stale vector Cut/Paste and both physical deletion key codes.
# Clipboard storage remains the desktop's separate session service.
vectorclipchecks()
{
    owner := panes->start(panes->Vector,"","");
    svg := "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 1000 600\"><path fill=\"#245ac4\" stroke=\"none\" d=\"M 100 100 L 300 100 L 200 300 Z\"/></svg>";
    pasted := cliprequest(owner,panes->PasteSelection,svg,
        ref Uibridge->Message(Uibridge->Request,0,0,0,0,0,""));
    require(pasted.status == 0 && pasted.revision == 1 && pasted.rangelast == 1,
            "typed SVG paste selects inserted figure");
    exported := call(owner,panes->ExportSelection,nil);
    require(exported.transfer != "" && exported.rangefirst == 0 && exported.rangelast == 1,
            "vector Copy exports immutable selected SVG through ordinary clipboard protocol");
    refused := cliprequest(owner,panes->CutSelection,"",
        ref Uibridge->Message(Uibridge->Request,0,0,0,0,1,""));
    require(refused.status == Idecore->Conflict && refused.revision == 1,
            "stale vector Cut cannot remove a figure");
    cut := cliprequest(owner,panes->CutSelection,"",
        ref Uibridge->Message(Uibridge->Request,0,0,1,0,1,""));
    require(cut.status == 0 && cut.revision == 2 && cut.rangefirst == cut.rangelast,
            "fresh Cut removes complete selected figure");
    pasted = cliprequest(owner,panes->PasteSelection,exported.transfer,
        ref Uibridge->Message(Uibridge->Request,0,0,2,0,0,""));
    deleted := call(owner,panes->Input,ref Uibridge->Message(Uibridge->Key,0,46,1,0,0,""));
    require(pasted.status == 0 && deleted.status == 0 && deleted.revision == 4 &&
            deleted.rangefirst == deleted.rangelast,"Delete key removes selected figure");
    pasted = cliprequest(owner,panes->PasteSelection,exported.transfer,
        ref Uibridge->Message(Uibridge->Request,0,0,4,0,0,""));
    deleted = call(owner,panes->Input,ref Uibridge->Message(Uibridge->Key,0,8,1,0,0,""));
    require(pasted.status == 0 && deleted.status == 0 && deleted.revision == 6,
            "Backspace key removes selected figure");
    call(owner,panes->Shutdown,nil);
}
