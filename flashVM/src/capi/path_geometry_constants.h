/* FlashVM: private path geometry representation and measurement admission limits. */
#ifndef FVM_PATH_GEOMETRY_CONSTANTS_H
#define FVM_PATH_GEOMETRY_CONSTANTS_H /* Guard for private path geometry constants. */
/* Maximum commands accepted by one mutable public path. */
#define FVM_PATH_GEOMETRY_MAX_COMMANDS 1000000UL
/* Initial geometric capacity used when appending transactional path fragments. */
#define FVM_PATH_GEOMETRY_INITIAL_APPEND_CAPACITY 8U
/* A quadratic command contains one control and its endpoint. */
#define FVM_PATH_GEOMETRY_QUADRATIC_POINTS 2U
/* A cubic command contains two controls and its endpoint. */
#define FVM_PATH_GEOMETRY_CUBIC_POINTS 3U
/* Maximum positive stroke width admitted by backend-independent geometry queries. */
#define FVM_PATH_GEOMETRY_MAX_STROKE_WIDTH 1000000.0
/* Absolute path coordinate bound, shared with public path admission. */
#define FVM_PATH_GEOMETRY_MAX_COORDINATE 1000000.0
/* Relative vectors scale to fewer than 25 magnitude bits; products stay below 2^50. */
#define FVM_PATH_GEOMETRY_LINEAR_VECTOR_BITS 25
/* Binary radix is required for exact multiplication by the dyadic lattice scale. */
#define FVM_PATH_GEOMETRY_LINEAR_RADIX 2
/* Integer products and their differences are exact on the admitted coordinate grid. */
#define FVM_PATH_GEOMETRY_LINEAR_MANTISSA_BITS 53
#endif
