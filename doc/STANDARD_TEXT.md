# ActionScript text bridge (Flash 6–8 subset)

The opt-in `fvm_standard_text_install` adds TextFormat/TextField class metadata
and a named TextFormat constructor. Install it separately from the initial
numeric library. It rejects conflicting bindings and validates registrations
on repeat calls. No libraries are downloaded or linked dynamically.

## Implemented surface

| Entry | Tested behavior |
| --- | --- |
| AVM1 NewObject / fvm_construct | Dedicated named native constructor registry, ordered arguments, object-result validation |
| Native properties | Own/prototype getters and setters, original receiver binding, read-only ignored writes, GC-traced captures |
| new TextFormat | Thirteen positional parameters; missing properties are null; additional Flash 8 fields are initially null |
| TextFormat.getTextExtent | Six numeric metrics, optional measured wrapping, 4-pixel total gutter, font size and leading |
| Host-created TextField | text, multiline, wordWrap, _width, _height, textColor and read-only length/textWidth/textHeight |
| Formatting methods | Whole-field getTextFormat/setTextFormat and getNewTextFormat/setNewTextFormat snapshots |
| Geometry | Independent editable text-field outline including the 2-pixel origin gutter |

The naming/constructor order, null formatting properties and extent gutter
contract follow the [Adobe AS2 TextFormat reference mirror](https://open-flash.github.io/mirrors/as2-language-reference/TextFormat.html).
The field API surface follows the [Adobe AS2 TextField reference mirror](https://open-flash.github.io/mirrors/as2-language-reference/TextField.html).
These references do not establish complete Flash behavior or pixel parity for
this implementation. Native unhinted external font metrics define the current
geometry; SWF DefineFont/DefineText/DefineEditText integration is not implemented.

## Fonts and ownership

Register TTF/OpenType bytes by name with fvm_standard_text_register_font. Input
is copied and validated. A context holds at most 16 named faces / 64 MiB of source
bytes until destruction. Names are at most 255 bytes; duplicate registration
rejects replacement. The first registered name is the default. There is no
system-font lookup/fallback. Size selects a new unhinted FreeType face through
the tested native font API. Registries are context-local and are not implicitly
copied into ActionScript worker contexts.

Create a field through fvm_textfield_create, then publish it with fvm_set_global
or keep it pinned/rooted. It is a GC-owned native resource, not a MovieClip
child/depth entry. fvm_textfield_path returns an independent path that survives
collection of the field and destruction of the context. The host draws or masks
it with AGG/Cairo through path.h and chooses the color, e.g. from textColor.
The exported contours are not clipped to the field rectangle.

Setter text values are copied into context-owned strings. Previously returned
text string snapshots remain valid until context destruction, even after later
text assignments or field collection. Object pointers, unlike these strings,
require a live GC root/pin. Constructor/accessor captures are traced, including
prototype accessors; replacing registrations drops previous capture roots.
Callbacks run under the native guard and cannot reenter GC. Callback exceptions
are contained at the C API boundary. Objects with native accessors remain
unsupported for channel graph transfer rather than losing their callbacks.

## Formatting and layout policy

Applied formatting currently supports font, size (1–4096), RGB color, left /
center / right alignment, and leading. Leading is added to ascent + descent;
negative leading is accepted when resulting baseline spacing stays positive.
Explicit null/undefined properties preserve the previous style. Enabled kerning
uses HarfBuzz; disabling it is unsupported. Nonzero letterSpacing/margins/indents,
bold/italic/underline/bullets, hyperlinks, tab stops and justify are rejected
when the format is applied or measured. Storing these values in TextFormat is
allowed; silent simulated rendering of them is not.

setTextFormat changes current text; setNewTextFormat changes future text.
Replacing text uses a copy of the future format. get methods return independent
TextFormat objects. Unimplemented properties in these snapshots remain null;
this is an explicit difference from complete AS2 getTextFormat results.
Character-range formatting overloads return UNSUPPORTED. Numeric formatting
uses the existing primitive ToNumber subset; text requires a UTF-8 string and
multiline/wordWrap require booleans. Full AS coercion and SWF-version gating are
not claimed. RGB colors are integral 0..0xffffff; dimensions are finite
0..1,000,000. Unknown user properties remain ordinary object data.

Wrapping is enabled when both multiline and wordWrap are true, within width-4.
For nonpositive inner width, a 1/64-pixel wrapping width still makes progress.
Without wrapping, center/right alignment is applied when exporting contours.
Metrics exclude alignment offsets and gutters. _height stores the field box;
it does not clip, scroll or resize glyphs. Hard line separators in single-line
layout return UNSUPPORTED rather than inventing untested Flash behavior.
length counts UTF-16 code units, while native shaping clusters remain UTF-8
byte positions. See TEXT_LAYOUT.md for greedy wrapping, whitespace, work limits
and script-itemization restrictions. Tabs/soft hyphens and unsupported layout
requests can be stored as text but fail when measured/exported.

Known unsupported display/input properties (html/htmlText, autoSize, border,
background, embedFonts, password, selectable, scrolling, antialias settings,
filters and display transforms) explicitly return UNSUPPORTED on access/write.
Likewise replaceText/replaceSel, listener and display-node methods are blocked.
There is no timeline, MovieClip.createTextField, HTML parser, styled spans,
selection/caret/input editing, font fallback, SWF embedded font decoding or
complete display list in this checkpoint. Direct new TextField is unsupported;
host construction is the explicit temporary bridge.

## Profiles and example

FONTS enables font registration, measurement and field creation; NO_FONTS still
supports TextFormat construction/property storage, and reports UNSUPPORTED for
font operations. NO_GRAPHICS retains measurement and editable paths.
examples/embedding/standard_text/main.c demonstrates the complete supported
host flow. tests/standard_textformat.c constructs/calls from AVM1;
tests/standard_textfield.c uses AVM1 GetMember/SetMember and tests resource GC.
Native constructor/accessor tests are in tests/native_properties.c.
See STANDARD_TEXT_CHECKPOINT.md for build/test evidence and mirror status.
