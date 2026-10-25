# Reviewed local source adaptations

`inferno-byte-array-descriptors.patch` targets
bhgv/Inferno-OS-bhgv experiment revision `96935b070dfb28c18ce9ce35c0bcf7b2b96a63bc`.
It permits legal module-local byte-array descriptors in string/UTF conversion,
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
