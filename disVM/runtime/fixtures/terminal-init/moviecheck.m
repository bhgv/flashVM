# Copyright (c) 2026 bhgv
# EN: Private typed acceptance helper for the inherited movie service.
# RU: Private typed acceptance helper для inherited movie service.

Moviecheck: module
{
    run: fn(term: Terminal, session: ref Terminal->Session); # Verify real code/data records.
    scope: fn(term: Terminal, session: ref Terminal->Session); # Verify child FORKNS and held grants.
    denied: fn(term: Terminal, session: ref Terminal->Session); # Verify live unmount refusal.
};
