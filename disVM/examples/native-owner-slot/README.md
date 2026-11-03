# Publishing an owned reference through a native slot

`example.c` provides two small extension helpers. The sender exports its existing
owner-local entry into a private transfer, exchanges that owner into the shared
slot, then cancels the previous owner outside the mutex. On publication refusal
it restores the original entry. The receiver obtains a distinct retained owner
directly into its local scope. It never reads a raw pointer before retaining it.

The source entry must already own its object. Each receiver opens its own scope
on its own bound context. Open scopes close before context unbind, including
native error cleanup supported by the host. A slot address requires separate
publication/lifetime control: remove its owner and join all users before destroy.
The helper aborts on a broken rollback/retirement protocol; returning would lose
the transfer's existing owner. Ordinary publication refusal returns zero after
restoring the original owner.

Compile these functions into a native extension with the installed Dis headers
and link the SDK native ownership/port implementation. The file is C89/C++98
compatible. No guest import or automatic native-module registration is implied.

For a complete threaded executable with immutable native payload, run the
[owner-slot fixture](../../tests/owner_slot.c) through its Python driver. For the
actual managed String/root/Fast example, use the hosted runtime's
[managed-owner probe](../../runtime/managed_owner_probe.c). These use the same
production slot implementation, not replacement models.

[Exact API/lock contracts](../../runtime/OWNER-SLOTS.md).
