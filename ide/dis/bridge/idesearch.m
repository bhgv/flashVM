# Reusable literal search/replacement over the existing channel-owned document.
Idesearch: module
{
    PATH: con "idesearch.dis"; # No renderer, filesystem or private document pointer dependency.

    # Find the next literal match from a source scalar, wrapping at most once.
    # core/document are borrowed, revision is expected content revision; returns real owner reply.
    # Conflict/closed/range/empty-needle errors never change selection, source or history.
    next: fn(core: Idecore, document: chan of Idecore->Request,
             revision, position: int, needle: string): Idecore->Reply;

    # Replace every nonoverlapping literal match through one revision-bound owner operation.
    # Returns the real reply and match count; success is one Undo entry, errors preserve history.
    replace: fn(core: Idecore, document: chan of Idecore->Request,
                revision: int, needle, replacement: string): Idecore->Reply;
};
