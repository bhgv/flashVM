# Gradient spread modes

The additive [`fvm_path_draw_gradient_spread`](../include/flashvm/gradient.h#L49) C89 entry accepts an unsigned selector: [`FVM_GRADIENT_SPREAD_PAD`](../include/flashvm/gradient.h#L19) (0), [`FVM_GRADIENT_SPREAD_REFLECT`](../include/flashvm/gradient.h#L20) (1), or [`FVM_GRADIENT_SPREAD_REPEAT`](../include/flashvm/gradient.h#L21) (2). [`FVM_Gradient`](../include/flashvm/gradient.h#L30) retains its existing layout. The old draw entry continues to select PAD.

For the normalized sample ratio r, repeat uses r-floor(r), a half-open interval. Reflect reduces r modulo two, normalizes negative phases, and reverses the second interval. PAD keeps the existing endpoint and duplicate-stop behavior. Both linear and centered radial gradients use this policy before ordinary RGB, straight RGBA interpolation and the existing color transform, path coverage and compositing stages.

Inputs are borrowed for one synchronous call. No context, root or device is retained. Invalid selectors, stops, matrices and surfaces return INVALID_ARGUMENT before painting. A valid request in NO_GRAPHICS returns UNSUPPORTED. Allocation failure during preparation preserves destination pixels. Surface and affine limits remain those of the existing raster bridge.

`MovieClip.beginGradientFill` accepts the counted strings pad, reflect and repeat. Reflect/repeat require loaded SWF version 8; host-created contexts also accept them. Older loaded movies retain UNSUPPORTED for non-pad modes. Embedded NUL suffixes do not match an option. Existing interpolation and focal restrictions still apply.

The immutable gradient record stores optional numeric `spread` metadata only for non-pad modes. Missing metadata defaults to PAD. Copying, duplicateMovieClip, binary serialization and channel transfer preserve the scalar and existing record aliases. Receiving contexts need the updated renderer. Argument coercion can have script side effects; failure preserves the prior gradient and active path, without rolling back those side effects. No script callback runs during raster sampling.

## Validation and remaining work

Tests cover analytical signed linear/radial samples, periodic translations, seams, legacy PAD byte identity, version gates, counted names, copied caller data, duplicate ownership, GC, binary/channel transfers after sender destruction, allocation failures and coercion exceptions. These are host-side checks, not a claim of complete Flash reference compatibility.

DefineShape4, linearRGB, nonzero focal gradients and lineGradientStyle remain open. Existing DefineShape1/2/3 parsing rules are unchanged. All nine roadmap stages remain open. Real devices and other platforms require separate validation.


Gradient spread v0.1160–v0.1168 adds unsigned C89 PAD/REFLECT/REPEAT selection without changing [FVM_Gradient](../include/flashvm/gradient.h#L30) layout or the legacy PAD entry, plus Flash 8 beginGradientFill modes and optional copied graph metadata. Validation: 16 selected native gradient/quality/example tests pass; three focused ASan/UBSan tests pass with LeakSanitizer disabled; SDK-free NO_GRAPHICS C89 validation passes. Allocation sweeps preserve prior style and pixels on preparation failure; coercion errors retain explicit script side effects. New-mode JIT execution, worker concurrency, reference-player comparisons and physical/foreign platforms are not established by this series. DefineShape4, linearRGB, nonzero focal and lineGradientStyle remain open, as do all nine roadmap stages. [Contract](GRADIENT_SPREAD.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Gradient` | struct | [include/flashvm/gradient.h:30](../include/flashvm/gradient.h#L30) |
| `FVM_GRADIENT_SPREAD_PAD` | enum constant | [include/flashvm/gradient.h:19](../include/flashvm/gradient.h#L19) |
| `FVM_GRADIENT_SPREAD_REFLECT` | enum constant | [include/flashvm/gradient.h:20](../include/flashvm/gradient.h#L20) |
| `FVM_GRADIENT_SPREAD_REPEAT` | enum constant | [include/flashvm/gradient.h:21](../include/flashvm/gradient.h#L21) |
| `fvm_path_draw_gradient_spread` | function | [include/flashvm/gradient.h:49](../include/flashvm/gradient.h#L49) |
<!-- source-links:end -->
