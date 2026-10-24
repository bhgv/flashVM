# Idecore channel protocol

The public schema is [idecore.m](idecore.m); the sole state owner is `serve` in
[idecore.b](idecore.b). All document mutation uses the request channel. There is
no direct mutable Document handle or UI mutation API.

## Values, identity and ordering

`start()` creates one owner with closed document, revision 0, saved 0 and empty
text. Each invocation is an independent service identity. Requests and replies
are ADT **values**, copied at channel rendezvous, not shared mutable ADT refs.
Strings/channels remain Dis-managed references; the runtime's string value
semantics preserve earlier request/reply text across later edits. There is no
cross-VM heap sharing or native-pointer transport. A remote transport will need
its own serialization/handles; these local channels do not supply it automatically.

One owner receives/applies commands in channel rendezvous order. It sends one reply
before receiving another command. This serializes competing expected-revision
edits: only the first matching edit can advance that revision. Correlation IDs
are nonnegative caller values, echoed unchanged; they are not a replay cache,
transaction token or exactly-once guarantee. Distinct producers should coordinate
IDs if sharing a reply channel. Prefer a caller-owned reply channel per request.

Requests with nil reply channel are consumed and ignored with **no state effect**,
including Shutdown. Every other request is answered, including errors. A blocked
reply blocks this owner and bounds intake to one request. Callers must receive
their replies; cancellation, timeouts, subscriber flow control and lost-client
recovery are not implemented. Do not execute this receive/send loop on FlashVM's
render/event owner. A future host adapter needs an independent pump/budget.

## Operations

| Operation | Preconditions | State effect |
| --- | --- | --- |
| Snapshot | Valid nonnegative ID | Read state; ignore expected revision |
| Open | Closed; expected matches; text within limit | Increment revision, set text/open, saved=revision |
| Replace | Open; expected matches; valid `[first,last)` character range; result within limit | Replace text; increment revision only when content changes; saved unchanged |
| MarkSaved | Open; expected matches | saved=revision; no content revision increment |
| Close | Open; expected matches | Increment revision, discard text, close, saved=revision |
| Shutdown | Expected matches | Reply with current state, then terminate owner |

Open/Close always advance the service-lifetime revision. It does not reset at
reopen, so stale requests cannot accidentally match a replacement document.
MarkSaved acknowledges an already completed external save; it does **not** write
a file. Dirty is `opened && revision != saved`. A content-identical replacement
is a no-op and does not clear dirty state. Position units are Unicode characters
as measured by Limbo `len string`, not UTF-8 bytes, UTF-16 code units, graphemes,
lines or display columns. UI conversion and cursor/grapheme policy are future work.
Only Replace uses first/last; only Open/Replace use text.

The resulting length is checked as `insertion <= MaxCharacters - surviving`
after validating the range, avoiding additive overflow. MaxCharacters is 1048576.
Revision is capped at 2147483647; any required increment at that cap is rejected
without changing state. Snapshot, MarkSaved, identical Replace and Shutdown still
work at the cap. Shutdown is terminal: quiesce other producers first, receive its
reply, then stop using the channel. Dis has no channel-close notification here;
sending after shutdown can block forever. GC cleanup is owned by Dis after the
server returns, not a public free-pointer command.

## Failure precedence

Negative ID => Invalid. Unknown operation => Unknown. Snapshot then succeeds
independently of expected. Other operations reject a mismatch as Conflict before
operation-specific checks. Open rejects an existing document as Invalid, length
as TooLarge, then exhausted revision. Other document operations reject a closed
document as Closed. Replace checks range, length, no-op, then revision cap.
All rejected commands return the unchanged complete state; no partial edit occurs.
Channel nil-reply ignoring is separate from these acknowledged status outcomes.
