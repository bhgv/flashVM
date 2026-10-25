# Bundled editor font

`NotoSansMono-Regular.ttf` is an unmodified Noto Sans Mono face from
[notofonts/noto-fonts](https://github.com/notofonts/noto-fonts), pinned at
`ffebf8c1ee449e544955a7e813c54f9b73848eac`, source path
`hinted/ttf/NotoSansMono/NotoSansMono-Regular.ttf`.
[OFL.txt](OFL.txt) preserves the original copyright and SIL Open Font License 1.1.
[SOURCE.json](SOURCE.json) records exact font/license SHA-256 values.
The font remains under OFL and is not relabeled as the project's MIT code.

Direct builds and installed graphical packages place this directory beside the
entry Dis module. The runtime resolves the bundled face there without changing
caller cwd, network access, system font discovery or a font-location question.
`--font PATH` / `FVM_EDITOR_FONT` remains an optional explicit override.
The actual admitted bytes are retained privately so resizing does not reopen a
possibly replaced file. Default size is 11 pixels; Ctrl+wheel or two-finger pinch
requests a bounded Dis-owned size change in 6..48 pixels.

The face supports Latin, Greek and Cyrillic among its supplied Unicode glyphs;
it is not a complete all-Unicode/CJK/emoji fallback system. Missing glyphs use
the face's normal missing-glyph representation. Unicode source coordinates and
saved UTF-8 remain intact regardless of glyph coverage.

По-русски: свободный неизменённый Noto Sans Mono включён в сборку и пакет с OFL.
Редактор запускается без запроса шрифта, размер по умолчанию 11. Ctrl+колесо и
двухпальцевый щипок изменяют масштаб через Dis; файл шрифта повторно не открывается.
Полное покрытие всех знаков Unicode одним шрифтом и автоматический fallback не
заявляются, исходные Unicode-символы при этом не теряются.
