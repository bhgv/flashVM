# Live projection adapter for an actual Idecore document, not a synthetic object.
Idedocprojection: module
{
    PATH: con "idedocprojection.dis"; # Pure Limbo document/channel adapter.


    # Spawn an adapter borrowing the live document endpoint and loaded pure modules.
    # epoch/identity bind a session-authorized receiver, not a native address or filesystem path.
    # Returns nil on invalid inputs. Caller must Release before retiring the document endpoint.
    # Writes and Undo/Redo require request.number to equal the observed document revision.
    # [GC:] The adapter retains its ordinary Dis channel until Release, never the document heap ADT.
    start: fn(core: Idecore, values: Idevalue, document: chan of Idecore->Request,
              epoch: int, identity: string): chan of ref Ideprojection->Request;
};
