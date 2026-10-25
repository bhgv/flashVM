/* FlashVM: shared managed construction and AS2 arithmetic helpers for geometry classes. */
#ifndef FVM_GEOMETRY_SUPPORT_HPP
#define FVM_GEOMETRY_SUPPORT_HPP /* Include guard for internal geometry helpers. */
#include "geometry_state.hpp"

namespace fvm_geometry_detail
{ /* Property maps contain ordinary values and traced object edges. */
typedef fvm_detail::map<fvm_detail::string, FVM_Value> Properties;


/* Initialize a numeric value without restricting IEEE exceptional values. */
FVM_Value scalar(double);


/* [GC:] Initialize a managed object reference for a retained graph edge. */
FVM_Value managed(FVM_Object *);


/* Return an argument or undefined when its position is omitted. */
FVM_Value argument(unsigned int, const FVM_Value *, unsigned int);


/* Read a tuple of public fields in reverse order without coercing its values. */
FVM_Result fields(FVM_Context *, FVM_Object *, const char *const *, unsigned int, FVM_Value *);


/* Read and numerically coerce a tuple through public property dispatch. */
FVM_Result numbers(FVM_Context *, FVM_Object *, const char *const *, unsigned int, double *);


/* Numerically coerce one supplied or omitted argument. */
FVM_Result numeric_argument(FVM_Context *, unsigned int, const FVM_Value *, unsigned int, double &);


/* Validate an object argument's context before accessing its properties. */
FVM_Result object_argument(FVM_Context *, unsigned int, const FVM_Value *, unsigned int, FVM_Object *&);


/* Perform AS2 numeric addition or counted string concatenation. */
FVM_Result add(FVM_Context *, const FVM_Value &, const FVM_Value &, FVM_Value *);


/* Apply public += to one property without pre-reading other fields. */
FVM_Result offset(FVM_Context *, FVM_Object *, const char *, const FVM_Value &);


/* [GC:] Resolve a class through the native callback's retained binding graph. */
FVM_Result captured_class(FVM_Context *, void *, const char *, FVM_Object *&);


/* [GC:] Allocate and initialize a typed instance with its retained prototype. */
FVM_Result create(FVM_Context *,
                  FVM_Object *,
                  unsigned int,
                  const char *const *,
                  unsigned int,
                  const FVM_Value *,
                  const unsigned int *,
                  FVM_Value *);


/* [GC:] Create a Point using captured classes without requiring installed globals. */
FVM_Result new_point(FVM_Context *, void *, const FVM_Value *, FVM_Value *);
/* Assign numeric tuple values through public setter/watch dispatch. */
FVM_Result
write(FVM_Context *, FVM_Object *, const char *const *, unsigned int, const double *, const unsigned int * = 0);


/* Format public fields without losing raw strings, booleans or undefined. */
FVM_Result text(FVM_Context *, FVM_Object *, const char *const *, unsigned int, FVM_Value *);
} // namespace fvm_geometry_detail
#endif
