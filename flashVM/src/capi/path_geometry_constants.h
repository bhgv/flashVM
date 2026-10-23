/* FlashVM: private path geometry representation and measurement admission limits. */
#ifndef FVM_PATH_GEOMETRY_CONSTANTS_H
#define FVM_PATH_GEOMETRY_CONSTANTS_H /* Guard for private path geometry constants. */
/* A quadratic command contains one control and its endpoint. */
#define FVM_PATH_GEOMETRY_QUADRATIC_POINTS 2U
/* A cubic command contains two controls and its endpoint. */
#define FVM_PATH_GEOMETRY_CUBIC_POINTS 3U
/* Maximum positive stroke width admitted by backend-independent geometry queries. */
#define FVM_PATH_GEOMETRY_MAX_STROKE_WIDTH 1000000.0
#endif
