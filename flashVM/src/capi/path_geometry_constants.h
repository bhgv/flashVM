/* FlashVM: private path geometry representation and measurement admission limits. */
#ifndef FVM_PATH_GEOMETRY_CONSTANTS_H
#define FVM_PATH_GEOMETRY_CONSTANTS_H /* Guard for private path geometry constants. */
/* A quadratic command contains one control and its endpoint. */
#define FVM_PATH_GEOMETRY_QUADRATIC_POINTS 2U
/* A cubic command contains two controls and its endpoint. */
#define FVM_PATH_GEOMETRY_CUBIC_POINTS 3U
/* Maximum positive stroke width admitted by backend-independent geometry queries. */
#define FVM_PATH_GEOMETRY_MAX_STROKE_WIDTH 1000000.0
/* Absolute path coordinate bound, shared with public path admission. */
#define FVM_PATH_GEOMETRY_MAX_COORDINATE 1000000.0
/* Exact dyadic lattice used to certify non-axis collinearity. */
#define FVM_PATH_GEOMETRY_LINEAR_GRID 16.0
/* Integer products and their differences are exact on the admitted coordinate grid. */
#define FVM_PATH_GEOMETRY_LINEAR_MANTISSA_BITS 53
#endif
