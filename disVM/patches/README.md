# Reviewed local source adaptations

`inferno-byte-array-descriptors.patch` targets
bhgv/Inferno-OS-bhgv experiment revision `3c6f5dca823927d8cec8c86159ccc35029b3bd51`.
The mail patch includes the production change and WORD32/native UBSan string/UTF regressions.
Author and committer: `bhgv <bhgv@bhgv.bhgv>`. It permits legal module-local byte-array descriptors in string/UTF conversion,
while retaining the canonical Tbyte fast path and rejecting pointer-bearing or
wrong-size element descriptors. New arrays emitted by Limbo can carry their
module's equivalent one-byte, pointer-free type rather than the Tbyte address.

The runtime applies the same change to generated source views. Imported files
remain exact and hash-verifiable; this patch has not been published upstream.
The native workspace regression exposed the error. An additional UBSan test
runs actual retained string functions with the module-local descriptor across
all their existing alignment/alias cases in WORD32 and WORD64.

NC64-specific alt/header, formatting-width, constant-sign and ABI changes belong
only to the experimental runtime view and are deliberately excluded from this
upstream patch. The old yacc parser's sentinel adaptation is in the selected
bootstrap build, not an alteration of retained upstream source files.

## v0.1550 upstream reconciliation

Both selected manifests now pin public experiment `3c6f5dc`. Upstream already
contains the dtoa 32-bit-limb/unsigned-pack fix, complete big-constant identities
and both Styx frame-header guards; source preparation verifies those fixes and
preserves the imported bytes instead of applying them twice. New byte-slice JIT
and channel-owner/index checks are included by the normal vendor refresh.

The descriptor patch is prepared as local Inferno commit `416c2e1` on top of that
public revision. Publication was blocked by automatic approval review; it is
**not** included in the public upstream pin. The FlashVM runtime retains its
explicit derived-source adaptation pending publication. The native UI boundary
also accepts equivalent pointer-free byte descriptors, allowing real
compiler-produced channel frames without exposing managed pointers.
