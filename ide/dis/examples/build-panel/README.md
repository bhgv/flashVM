# Existing-document build tutorial

`main.b` runs on our Dis interpreter/JIT and compiles one bounded numeric Limbo
module through `Idebuildpanel`. Every line explains the caller's responsibility:
open an editor document, capture its revision, start private compilation services,
request optimized Dis, check both admission and compiler stage, fetch copied
binary/debug arrays, stop the panel, and finally stop the separately owned editor.
No file is created and no result is loaded or executed.

Run `disvm --jit build-panel.dis` or `disvm --no-jit build-panel.dis` beside a
matching installed module collection with `Fircompiler=ON`. Success prints
`PASS build panel tutorial: revision=ok binary=ok debug=ok owners=closed`.
An OFF host reports the missing capability after retiring its document.

Change the explicit target to `Fircompiler->Fir` or `Fircompiler->Swf` to request
a different format. Change the source and language together for AS1/AS2/AS3.
These are bounded frontends, not complete language implementations. Artifact
metadata reports its actual format; never relabel bytes by changing a filename.
For an editor, rebuild after each source revision and use Diagnostic to obtain a
current `Idecore.Utf8Span` character range. Old revisions cannot fetch results.

The full service, concurrency/ownership limits and graphical F7/F8 controls are
documented in `BUILD-PANEL.md` in the source IDE directory or installed SDK.
