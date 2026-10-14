/* FlashVM: managed object/resource identities shared by runtime and graph transport. */
#ifndef FVM_RESOURCE_KINDS_H
#define FVM_RESOURCE_KINDS_H /* Header guard. */
/* Unmanaged-layout ordinary table/object; no attached resource kind. */
#define FVM_RESOURCE_PLAIN_OBJECT 0
/* Managed identity for channel. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_CHANNEL 1
/* Managed identity for worker. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_WORKER 2
/* Managed identity for gc namespace. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_GC_NAMESPACE 3
/* Managed identity for thread namespace. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_THREAD_NAMESPACE 4
/* Managed identity for channel namespace. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_CHANNEL_NAMESPACE 5
/* Managed identity for userdata. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_USERDATA 6
/* Managed identity for mutex. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_MUTEX 8
/* Managed identity for math namespace. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_MATH_NAMESPACE 10
/* Managed identity for boolean. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_BOOLEAN 20
/* Managed identity for boolean class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_BOOLEAN_CLASS 21
/* Managed identity for error. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_ERROR 22
/* Managed identity for error class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_ERROR_CLASS 23
/* Managed identity for number. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_NUMBER 24
/* Managed identity for number class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_NUMBER_CLASS 25
/* Managed identity for string. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_STRING 26
/* Managed identity for string class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_STRING_CLASS 27
/* Managed identity for array. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_ARRAY 28
/* Managed identity for array class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_ARRAY_CLASS 29
/* Managed identity for object. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_OBJECT 30
/* Managed identity for object class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_OBJECT_CLASS 31
/* Managed identity for script function. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_SCRIPT_FUNCTION 32
/* Managed identity for function class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_FUNCTION_CLASS 33
/* Managed identity for date. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_DATE 34
/* Managed identity for date class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_DATE_CLASS 35
/* Managed identity for native function. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_NATIVE_FUNCTION 36
/* Managed identity for super. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_SUPER 37
/* Managed identity for movieclip. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_MOVIECLIP 40
/* Managed identity for movieclip class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_MOVIECLIP_CLASS 41
/* Managed identity for point. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_POINT 42
/* Managed identity for matrix. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_MATRIX 43
/* Managed identity for point class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_POINT_CLASS 44
/* Managed identity for matrix class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_MATRIX_CLASS 45
/* Managed identity for rectangle. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_RECTANGLE 46
/* Managed identity for rectangle class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_RECTANGLE_CLASS 47
/* Managed identity for color transform. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_COLOR_TRANSFORM 48
/* Managed identity for color transform class. Preserve this numeric ID for existing graph
 * transport. */
#define FVM_RESOURCE_COLOR_TRANSFORM_CLASS 49
/* Managed identity for transform. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_TRANSFORM 50
/* Managed identity for transform class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_TRANSFORM_CLASS 51
/* Managed identity for bitmap data. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_BITMAP_DATA 52
/* Managed identity for bitmap data class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_BITMAP_DATA_CLASS 53
/* Managed identity for color matrix filter. Preserve this numeric ID for existing graph transport.
 */
#define FVM_RESOURCE_COLOR_MATRIX_FILTER 54
/* Managed identity for color matrix filter class. Preserve this numeric ID for existing graph
 * transport. */
#define FVM_RESOURCE_COLOR_MATRIX_FILTER_CLASS 55
/* Managed identity for bitmap filter. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_BITMAP_FILTER 56
/* Managed identity for bitmap filter class. Preserve this numeric ID for existing graph transport.
 */
#define FVM_RESOURCE_BITMAP_FILTER_CLASS 57
/* Managed identity for text format. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_TEXT_FORMAT 1001
/* Managed identity for text field. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_TEXT_FIELD 1002
/* Managed identity for text format class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_TEXT_FORMAT_CLASS 1003
/* Managed identity for text field class. Preserve this numeric ID for existing graph transport. */
#define FVM_RESOURCE_TEXT_FIELD_CLASS 1004
/* Stable managed identity for blur filter. */
#define FVM_RESOURCE_BLUR_FILTER 58
/* Stable managed identity for blur filter class. */
#define FVM_RESOURCE_BLUR_FILTER_CLASS 59
/* Stable managed identity for convolution filter. */
#define FVM_RESOURCE_CONVOLUTION_FILTER 60
/* Stable managed identity for convolution filter class. */
#define FVM_RESOURCE_CONVOLUTION_FILTER_CLASS 61
#endif
