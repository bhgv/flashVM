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

## Revision-bound source navigation (0.3.0)

Locate and Offset are read-only document requests. They require the current
expected revision, an open document and a nonnegative correlation ID. Usual
Conflict/Closed precedence applies. Neither operation changes text, revision or
saved state, including at MaxRevision. Both return the same full state snapshot.

- Locate: first is a character offset in [0, len text], including EOF; last/text
  are ignored. Returns offset, one-based line and zero-based character column.
- Offset: first is a one-based line, last a zero-based character column; text is
  ignored. The column may point to line end (before LF), never across LF.
  The final empty line after a trailing LF is valid.

LF alone starts a line. CR counts as an ordinary character, including in CRLF.
Every character boundary therefore round-trips, even the boundary between CR and
LF. Astral characters count once; combining marks count separately. These are
Limbo string units, not UTF-8 bytes, UTF-16 units, graphemes or screen columns.
Compiler/debug producer positions need an explicit conversion before these calls.

Reply appends offset/line/column, each -1 for errors and all other operations.
This changes the Limbo interface signature: rebuild core and every client against
the same idecore.m; mixing 0.2.x clients with 0.3.x modules is unsupported. Existing
operation and status numbers retain their values. There is no live ABI upgrade.
Navigation scans at most MaxCharacters characters with constant extra storage;
a long query holds this owner until its reply. Never pump it on the render owner.

## Exact range search (0.3.1)

Find requires an open document and exact expected revision, like Locate/Offset.
first/last select a half-open character range [first,last) within the document;
text is a nonempty literal pattern. Range errors precede empty-pattern Invalid.
Find returns the first exact case-sensitive occurrence wholly inside that range,
with its offset/line/column and complete unchanged state. No match returns the
new appended NotFound status with coordinate fields -1. Empty patterns are Invalid;
there is no implicit wrap, regex, case folding or Unicode normalization. A match
may include LF/CRLF and combining characters. The caller knows the selection's
length from the request's pattern and must keep its revision before applying it.

Prefix-table matching takes O(range length + pattern length), plus at most one
document prefix scan for the successful coordinate conversion. It uses O(pattern
length) private Dis-managed integers, bounded by MaxCharacters. No prefix table
is allocated when the pattern cannot fit. The request still occupies the sole
owner until its reply; asynchronous cancellation and render budgets remain open.
