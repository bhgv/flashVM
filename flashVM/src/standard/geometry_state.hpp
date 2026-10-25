/* FlashVM: managed geometry class kinds, namespace installation and stable graph callbacks. */
#ifndef FVM_GEOMETRY_STATE_HPP
#define FVM_GEOMETRY_STATE_HPP /* Include guard for geometry class integration. */
#include "../core/runtime.hpp"


/* Managed object kinds; no native backend handles are stored by these geometry objects. */
enum FVM_GeometryKind
{
    FVM_GEOMETRY_POINT = FVM_RESOURCE_POINT,               /* Point instance with ordinary x/y properties. */
    FVM_GEOMETRY_MATRIX = FVM_RESOURCE_MATRIX,             /* Matrix instance with ordinary affine properties. */
    FVM_GEOMETRY_POINT_CLASS = FVM_RESOURCE_POINT_CLASS,   /* Callable Point constructor. */
    FVM_GEOMETRY_MATRIX_CLASS = FVM_RESOURCE_MATRIX_CLASS, /* Callable Matrix constructor. */
    FVM_GEOMETRY_RECTANGLE = FVM_RESOURCE_RECTANGLE, /* Rectangle instance with ordinary coordinate/extent fields. */
    FVM_GEOMETRY_RECTANGLE_CLASS = FVM_RESOURCE_RECTANGLE_CLASS, /* Callable Rectangle constructor. */
    FVM_GEOMETRY_COLOR_TRANSFORM =
        FVM_RESOURCE_COLOR_TRANSFORM, /* Native numeric color channels stored in a traced state object. */
    FVM_GEOMETRY_COLOR_TRANSFORM_CLASS = FVM_RESOURCE_COLOR_TRANSFORM_CLASS, /* Callable ColorTransform constructor. */
    FVM_GEOMETRY_TRANSFORM =
        FVM_RESOURCE_TRANSFORM, /* Native scene-transform reference with traced MovieClip storage. */
    FVM_GEOMETRY_TRANSFORM_CLASS = FVM_RESOURCE_TRANSFORM_CLASS /* Callable Transform constructor. */
};


/* [GC:] Stage flash.geom namespace/classes in unpublished standard-library maps. */
FVM_Result fvm_geometry_stage(FVM_Context *,
                              fvm_detail::map<fvm_detail::string, FVM_Value> &,
                              fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);


/* Validate the installed geometry namespace without invoking script. */
FVM_Result fvm_geometry_validate(FVM_Context *);


/* Identify geometry callbacks using stable IDs independent of instance addresses. */
unsigned long fvm_geometry_callback_id(FVM_NativeFunction);


/* Restore an identified geometry callback without consulting user globals. */
FVM_NativeFunction fvm_geometry_callback_from_id(unsigned long);
#endif
