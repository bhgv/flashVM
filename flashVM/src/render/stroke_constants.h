/* FlashVM: private per-paint cap/join selectors, matching supported SWF LINESTYLE2 values. */
#ifndef FVM_RENDER_STROKE_CONSTANTS_H
#define FVM_RENDER_STROKE_CONSTANTS_H /* Include guard for private stroke selectors. */
/* Round cap extends a semicircle beyond each endpoint. */
#define FVM_STROKE_CAP_ROUND 0U
/* Butt cap ends at the endpoint without an extension. */
#define FVM_STROKE_CAP_BUTT 1U
/* Square cap extends half the stroke width beyond each endpoint. */
#define FVM_STROKE_CAP_SQUARE 2U
/* Round join fills the outer turn with a circular arc. */
#define FVM_STROKE_JOIN_ROUND 0U
/* Bevel join connects the outer edges with a straight segment. */
#define FVM_STROKE_JOIN_BEVEL 1U
#endif
