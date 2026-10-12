/* FlashVM: native Point/Matrix classes, namespace installation and stable graph callbacks. */
#ifndef FVM_GEOMETRY_STATE_HPP
#define FVM_GEOMETRY_STATE_HPP /* Include guard for geometry class integration. */
#include "../core/runtime.hpp"

/* Managed object kinds; no native backend handles are stored by these geometry objects. */
enum FVM_GeometryKind
{
    FVM_GEOMETRY_POINT = 42,       /* Point instance with ordinary x/y properties. */
    FVM_GEOMETRY_MATRIX = 43,      /* Matrix instance with ordinary affine properties. */
    FVM_GEOMETRY_POINT_CLASS = 44, /* Callable Point constructor. */
    FVM_GEOMETRY_MATRIX_CLASS = 45 /* Callable Matrix constructor. */
};

/* [GC:] Stage flash.geom namespace/classes in unpublished standard-library maps. */
FVM_Result fvm_geometry_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &,
                              fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);
/* Validate the installed geometry namespace without invoking script. */
FVM_Result fvm_geometry_validate(FVM_Context *);
/* Identify geometry callbacks using stable IDs independent of instance addresses. */
unsigned long fvm_geometry_callback_id(FVM_NativeFunction);
/* Restore an identified geometry callback without consulting user globals. */
FVM_NativeFunction fvm_geometry_callback_from_id(unsigned long);
#endif
