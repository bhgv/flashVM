/* FlashVM: C89 value-based execution services shared by interpreter and generated code.
 * No service accepts bytecode, Action codes or an interpreter operand stack. */
#ifndef FLASHVM_EXECUTION_H
#define FLASHVM_EXECUTION_H /* Public execution service guard. */
#include "value.h"
#include "matrix.h"
/* Context-local legacy register bank shared by generated and interpreted activations. */
#define FVM_EXEC_GLOBAL_REGISTER_COUNT 4U
/* Maximum explicit interface values accepted by the value-based declaration service. */
#define FVM_EXEC_MAX_INTERFACE_ARGUMENTS 65536U
/* Maximum argument count accepted by the bounded method execution service. */
#define FVM_EXEC_MAX_METHOD_ARGUMENTS 65535U
/* Maximum scene-frame offset accepted by semantic timeline-request preparation. */
#define FVM_EXEC_MAX_SCENE_FRAME_BIAS 65535UL
/* Preserve the unloaded named-constructor argument-vector bound. */
#define FVM_EXEC_MAX_CONSTRUCTOR_ARGUMENTS 65536U
/* Maximum positional parent-space limit values accepted by drag value preparation. */
#define FVM_EXEC_DRAG_BOUND_LIMIT 4U
/* Maximum explicit entries in an Array/Object literal, independent of bytecode layout. */
#define FVM_EXEC_MAX_LITERAL_ENTRIES 65536U
#ifdef __cplusplus
extern "C"
{
#endif
    /* One object-literal entry in source assignment order; both fields are borrowed inputs. */
    typedef struct FVM_ExecutionLiteralEntry
    {
        FVM_Value name;  /* Property name, converted before its corresponding assignment. */
        FVM_Value value; /* Property value, retained through constructor/name/setter callbacks. */
    } FVM_ExecutionLiteralEntry;
    /* Build an Array with count own elements in supplied order, without numeric length overload.
     * Count is bounded by MAX_LITERAL_ENTRIES; values may be null only for zero count.
     * Errors preserve out; out may alias an input. No bytecode or operand stack is accepted.
     * [GC:] Copies and roots all inputs and the result through construction/publication. */
    FVM_API FVM_Result fvm_exec_literal_array(FVM_Context *, unsigned int count,
                                               const FVM_Value *values, FVM_Value *out);
    /* Construct Object, then coerce names and assign entries in supplied order.
     * Loaded SWFs use AVM1 text conversion and ignore converted empty names; hosts retain them.
     * Count has the same bound as Array literals. Null entries require zero count.
     * Errors preserve out, but callback effects and earlier property writes are not rolled back.
     * Out may alias an entry field. [GC:] All input values/names and temporary results are rooted. */
    FVM_API FVM_Result fvm_exec_literal_object(FVM_Context *, unsigned int count,
                                                const FVM_ExecutionLiteralEntry *entries,
                                                FVM_Value *out);
    /* Query a native clip's physical world matrix using per-ancestor binary32 composition.
     * Local geometry follows existing native getter semantics; root locks do not change it.
     * Clip must be an owned native instance. Errors preserve output; temporary roots and a
     * native GC barrier protect callbacks. No bytecode or interpreter stack is required. */
    FVM_API FVM_Result fvm_exec_clip_world_matrix(FVM_Context *, FVM_Object *clip,
                                                   FVM_TwipMatrix *out);
    /* Convert an object-valued point in place: reverse=0 local to world, nonzero world
     * to local. Writes x then y; setter errors may leave x changed. Loaded SWFs read
     * only own numeric x/y fields, ignore invalid/missing fields and use an unchanged
     * point for singular inverse. Host mode retains field coercion and strict errors.
     * Loaded inputs truncate/saturate to signed 32-bit twips (NaN becomes zero).
     * Loaded final application uses binary32 terms and nearest/even output twips.
     * Loaded ancestry/inverse/application use binary32 and rounded translation. Local
     * source geometry snapshots still use the existing native getter/double model. */
    FVM_API FVM_Result fvm_exec_clip_point(FVM_Context *, FVM_Object *clip, int reverse,
                                           const FVM_Value *point);
    /* Copy a native source clip's local matrix and color into destination atomically.
     * Both clips must belong to context and be native instances. Source geometry
     * getters follow existing native snapshot rules; their side effects are retained.
     * The destination keeps its own identity, children, parent and timeline. */
    FVM_API FVM_Result fvm_exec_clip_copy_transform(FVM_Context *, FVM_Object *destination,
                                                    FVM_Object *source);
    /* Poll worker cancellation and scheduled GC after accounting work. */
#define FVM_EXEC_POLL 1U
    /* [GC:] Account semantic instruction units in an active interpreted/generated activation.
     * location is a debugger mapping token; no instruction encoding is required. flags=0 only
     * accounts work and updates location. FVM_EXEC_POLL also polls stop requests and GC.
     * All live temporaries must be rooted before polling. Return failure immediately; generated
     * code must not ignore exhausted budget/cancellation statuses. A zero cost can poll alone.
     * Top-level entry resets the budget; nested interpreter/generated calls share it. */
    FVM_API FVM_Result fvm_exec_checkpoint(FVM_Context *, unsigned long units,
                                           unsigned long location, unsigned int flags);
    /* Numeric operations; values are ABI identifiers, not AVM1 instruction codes. */
    typedef enum FVM_ExecutionNumeric
    {
        FVM_EXEC_ADD = 1,       /* Legacy numeric addition, including metamethods. */
        FVM_EXEC_SUBTRACT,      /* Numeric subtraction. */
        FVM_EXEC_MULTIPLY,      /* Numeric multiplication. */
        FVM_EXEC_DIVIDE,        /* Numeric division, including pre-SWF5 #ERROR#. */
        FVM_EXEC_REMAINDER,     /* Floating point remainder. */
        FVM_EXEC_BIT_AND,       /* Signed 32-bit AND. */
        FVM_EXEC_BIT_OR,        /* Signed 32-bit OR. */
        FVM_EXEC_BIT_XOR,       /* Signed 32-bit XOR. */
        FVM_EXEC_SHIFT_LEFT,    /* Masked 32-bit left shift. */
        FVM_EXEC_SHIFT_RIGHT,   /* Arithmetic right shift. */
        FVM_EXEC_SHIFT_UNSIGNED /* Logical right shift with SWF8 signed-result policy. */
    } FVM_ExecutionNumeric;
    /* Dynamic addition and comparison policies, separate from legacy numeric operations. */
    typedef enum FVM_ExecutionCompare
    {
        FVM_EXEC_ADD_DYNAMIC = 1, /* Primitive/string-aware addition. */
        FVM_EXEC_LESS,            /* Version-aware relational comparison. */
        FVM_EXEC_EQUAL,           /* Loose equality with coercion. */
        FVM_EXEC_STRICT_EQUAL,    /* Same-type equality without coercion. */
        FVM_EXEC_GREATER          /* Greater-than with reversed primitive conversion order. */
    } FVM_ExecutionCompare;
    /* Execute dynamic addition or comparison; output may alias either input. */
    FVM_API FVM_Result fvm_exec_compare(FVM_Context *, FVM_ExecutionCompare, const FVM_Value *,
                                        const FVM_Value *, FVM_Value *);
    /* Legacy numeric comparisons have different coercion rules from dynamic comparison. */
    typedef enum FVM_ExecutionLegacyCompare
    {
        FVM_EXEC_LEGACY_EQUAL = 1, /* Convert right before left, then compare numbers. */
        FVM_EXEC_LEGACY_LESS       /* Convert left before right, then compare numbers. */
    } FVM_ExecutionLegacyCompare;
    /* Logical operations return Boolean values under the loaded SWF version policy. */
    typedef enum FVM_ExecutionLogical
    {
        FVM_EXEC_LOGICAL_NOT = 1, /* Unary negation; right operand may be NULL. */
        FVM_EXEC_LOGICAL_AND,     /* Convert both operands left to right. */
        FVM_EXEC_LOGICAL_OR       /* Convert both operands left to right. */
    } FVM_ExecutionLogical;
    /* Compare with legacy numeric rules and optional comparison metamethods. */
    FVM_API FVM_Result fvm_exec_legacy_compare(FVM_Context *, FVM_ExecutionLegacyCompare,
                                               const FVM_Value *, const FVM_Value *, FVM_Value *);
    /* Execute a logical operation; outputs may alias either retained input. */
    FVM_API FVM_Result fvm_exec_logical(FVM_Context *, FVM_ExecutionLogical,
                                       const FVM_Value *, const FVM_Value *, FVM_Value *);
    /* Snapshot enumerable names into a native Array; loaded nonobjects return an empty Array.
     * No primitive boxing or target getters run; loaded snapshots avoid script dispatch.
     * Caller retains the result across later GC; host-only construction policy is preserved.
     * [GC:] Input remains rooted; failure leaves output intact, including aliases. */
    FVM_API FVM_Result fvm_exec_enumerate(FVM_Context *, const FVM_Value *, FVM_Value *);
    /* Delete an own member after name conversion; absent/inherited members report false. */
    FVM_API FVM_Result fvm_exec_delete_member(FVM_Context *, const FVM_Value *,
                                             const FVM_Value *, FVM_Value *);
    /* Built-in timeline commands independent of instruction encoding. */
    typedef enum FVM_ExecutionFrame
    {
        FVM_EXEC_NEXT_FRAME = 1, /* Advance one frame. */
        FVM_EXEC_PREVIOUS_FRAME, /* Rewind one frame. */
        FVM_EXEC_PLAY,           /* Enable timeline playback. */
        FVM_EXEC_STOP,           /* Disable timeline playback. */
        FVM_EXEC_GOTO_FRAME      /* Seek a numeric frame or label; playing selects playback. */
    } FVM_ExecutionFrame;
    /* [GC:] Services root validated operands and intermediates during synchronous execution.
     * Context supplies runtime objects, SWF version and host services only. Result aliases are
     * allowed; callers retain results across subsequent collections. Failures leave result intact.
     * Inputs and output pointers are mandatory; no callbacks may destroy the context. */
    /* Execute numeric semantics and preserve the operation's historical coercion order. */
    FVM_API FVM_Result fvm_exec_numeric(FVM_Context *, FVM_ExecutionNumeric, const FVM_Value *,
                                        const FVM_Value *, FVM_Value *);
    /* Frame-check policies are ABI commands, independent of wire Action identifiers. */
    typedef enum FVM_ExecutionFrameCheck
    {
        FVM_EXEC_FRAME_CHECK_DIRECT = 1, /* Exact unsigned direct frame request, no coercion. */
        FVM_EXEC_FRAME_CHECK_VALUE       /* String/integer request with legacy wrapping and slop. */
    } FVM_ExecutionFrameCheck;
    /* Test the currently selected native timeline after request conversion; absent targets are ready.
     * [GC:] Coercion is rooted; output may alias input and failures leave output intact. */
    FVM_API FVM_Result fvm_exec_frame_loaded(FVM_Context *, FVM_ExecutionFrameCheck,
                                              const FVM_Value *, FVM_Value *);
    /* Return a native display dot path after boxing; nonclips and the class prototype yield undefined.
     * [GC:] Retains the supplied value and boxed object; failures preserve aliased output. */
    FVM_API FVM_Result fvm_exec_target_path(FVM_Context *, const FVM_Value *, FVM_Value *);
    /* Read context-relative monotonic integer milliseconds modulo 2^32, without allocation.
     * Clock errors leave output and the previous elapsed sample intact; no GC roots are needed. */
    FVM_API FVM_Result fvm_exec_get_time(FVM_Context *, FVM_Value *);
    /* Return a bounded random integer after numeric conversion and saturating Int32 clamping.
     * Nonpositive/NaN bounds return zero without drawing; shares Math.random context state.
     * [GC:] Coercion roots input; failure preserves output, including aliases. */
    FVM_API FVM_Result fvm_exec_random_number(FVM_Context *, const FVM_Value *, FVM_Value *);
    /* Increment/decrement after version-aware numeric coercion, without integer wrapping.
     * [GC:] Retains input through coercion; failure leaves output intact, including aliases. */
    FVM_API FVM_Result fvm_exec_step(FVM_Context *, int decrement, const FVM_Value *, FVM_Value *);
    /* Read/write indexed display properties; write selects SetProperty and ignores output value. */
    FVM_API FVM_Result fvm_exec_display_property(FVM_Context *, int write, const FVM_Value *target,
                                                 const FVM_Value *index, const FVM_Value *value,
                                                 FVM_Value *result);
    /* Extract one-based string ranges; ordinary and MB variants share supported UTF-16 policy. */
    FVM_API FVM_Result fvm_exec_string_extract(FVM_Context *, const FVM_Value *input,
                                               const FVM_Value *start, const FVM_Value *length,
                                               FVM_Value *result);
    /* Concatenate after right-before-left string conversion, preserving counted bytes. */
    FVM_API FVM_Result fvm_exec_string_add(FVM_Context *, const FVM_Value *,
                                          const FVM_Value *, FVM_Value *);
    /* Explicit-string comparison accepts EQUAL/LESS/GREATER only. Greater converts
     * left first; other modes convert right first. Output is Boolean in every SWF version. */
    FVM_API FVM_Result fvm_exec_string_compare(FVM_Context *, FVM_ExecutionCompare,
                                              const FVM_Value *, const FVM_Value *, FVM_Value *);
    /* Count UTF-16 units after string coercion. Loaded SWF<6 non-ASCII remains unsupported. */
    FVM_API FVM_Result fvm_exec_string_length(FVM_Context *, const FVM_Value *, FVM_Value *);
    /* Read the first UTF-16 unit, replacing surrogates; empty strings yield zero. */
    FVM_API FVM_Result fvm_exec_char_code(FVM_Context *, const FVM_Value *, FVM_Value *);
    /* Wrap a coerced number to Uint16; zero yields empty, surrogates become replacement chars. */
    FVM_API FVM_Result fvm_exec_char_from_code(FVM_Context *, const FVM_Value *, FVM_Value *);
    /* Select a display target from a value; empty paths reset and undefined follows SWF policy. */
    FVM_API FVM_Result fvm_exec_select_target(FVM_Context *, const FVM_Value *);
    /* Coerce and send a URL request; flags follow the public FVM_AVM1_URL_* contract. */
    FVM_API FVM_Result fvm_exec_url(FVM_Context *, const FVM_Value *url, const FVM_Value *target,
                                    unsigned int flags);
    /* Coerce and deliver trace, including when its host sink is disabled. */
    FVM_API FVM_Result fvm_exec_trace(FVM_Context *, const FVM_Value *);
    /* Read or write a value member; loaded primitives use temporary wrappers after metadata hooks.
     * Native raw tables retain their indexed-slot policy. */
    FVM_API FVM_Result fvm_exec_member(FVM_Context *, int write, const FVM_Value *owner,
                                       const FVM_Value *member, const FVM_Value *value,
                                       FVM_Value *result);
    /* Invoke an already converted named member once; absent/noncallable values yield undefined.
     * [GC:] Retains receiver, defining prototype, function and arguments through getters/body. */
    FVM_API FVM_Result fvm_exec_invoke_member(FVM_Context *, FVM_Object *, const FVM_Value *name,
                                               unsigned int argc, const FVM_Value *argv, FVM_Value *out);
    /* Establish loaded prototype inheritance with superclass-before-subclass boxing.
     * [GC:] Retains operands, getter results and fresh metadata through subclass setters. */
    FVM_API FVM_Result fvm_exec_extends(FVM_Context *, const FVM_Value *subclass,
                                         const FVM_Value *superclass);
    /* Convert an ImplementsOp count; loaded objects yield zero without valueOf callbacks. */
    FVM_API FVM_Result fvm_exec_interface_count(FVM_Context *, const FVM_Value *,
                                                 size_t available, unsigned int *out);
    /* Install the first loaded SWF7+ interface prototype list without invoking getters.
     * Host calls retain legacy constructor-edge declarations; zero loaded entries are a noop. */
    FVM_API FVM_Result fvm_exec_implements(FVM_Context *, const FVM_Value *constructor,
                                            unsigned int count, const FVM_Value *interfaces);
    /* Test class/prototype identity; loaded operands use versioned boxing and own stored prototype.
     * [GC:] Retains original values through coercion; errors preserve output and aliasing is allowed. */
    FVM_API FVM_Result fvm_exec_instance_of(FVM_Context *, const FVM_Value *,
                                             const FVM_Value *constructor, FVM_Value *out);
    /* Preserve a matching value or return null; loaded primitive casts box for side effects.
     * [GC:] Retains original operands through constructor callbacks. Output may alias either. */
    FVM_API FVM_Result fvm_exec_cast(FVM_Context *, const FVM_Value *value,
                                      const FVM_Value *constructor, FVM_Value *out);
    /* Force native scene detachment, including authored/negative depths, without method lookup.
     * Accepts only native clip instances owned by the context. Repeated detachment is allowed;
     * a parentless root is stopped. This is not historical Action RemoveSprite policy and does
     * not resolve targets, dispatch unload events or alter the activation's selected target.
     * [GC:] Retains the operand; allocation errors preserve parent/slot/binding/playback links. */
    FVM_API FVM_Result fvm_exec_detach_clip(FVM_Context *, FVM_Object *);
    /* Read this native instance's committed detach/replacement marker, bypassing script fields.
     * A new parentless root is false; descendants are not implicitly marked by this query.
     * Legacy state without a marker reads false. Errors preserve the caller output. */
    FVM_API FVM_Result fvm_exec_clip_detached(FVM_Context *, FVM_Object *, FVM_Value *out);
    /* Read intrinsic _lockroot, or write the versioned Boolean conversion of input.
     * Root locks affect root/path lookup without changing parent edges. Errors preserve output
     * and stored state; output may alias input. No interpreter activation or bytecode is used. */
    FVM_API FVM_Result fvm_exec_clip_lock_root(FVM_Context *, int write, FVM_Object *,
                                                const FVM_Value *input, FVM_Value *out);

    /* Remove an attached native instance at nonnegative public depth; otherwise succeed unchanged.
     * Detached ancestors suppress removal. Uses intrinsic state without script method lookup.
     * This explicit-object policy does not resolve Action target paths or deliver unload events. */
    FVM_API FVM_Result fvm_exec_remove_dynamic_clip(FVM_Context *, FVM_Object *);
    /* Optional normalized drag limits in the dragged clip's parent's pixel space. */
    typedef struct FVM_ExecutionDragBounds
    {
        double x_min; /* Minimum parent-space X; nonfinite input becomes zero. */
        double y_min; /* Minimum parent-space Y; nonfinite input becomes zero. */
        double x_max; /* Maximum parent-space X; reversed limits are swapped. */
        double y_max; /* Maximum parent-space Y; reversed limits are swapped. */
    } FVM_ExecutionDragBounds;
    /* Prepare script-style drag options: optional versioned Boolean center plus 0..4 bounds.
     * Any supplied bound enables constraints; missing trailing limits convert from undefined.
     * Numeric conversion order is xMin/yMin/xMax/yMax. Extra counts/foreign inputs are invalid.
     * [GC:] Retains clip/all values through collecting conversions before replacing active state. */
    FVM_API FVM_Result fvm_exec_drag_values(FVM_Context *, FVM_Object *, const FVM_Value *center,
                                             unsigned int bound_count, const FVM_Value *bounds);

    /* Begin/replace native dragging without moving yet; NULL bounds means unconstrained.
     * lock_center selects pointer-centered positioning; otherwise motion uses mouse deltas.
     * [GC:] The active instance is retained as a context root until end/removal. */
    FVM_API FVM_Result fvm_exec_drag_begin(FVM_Context *, FVM_Object *, int lock_center,
                                            const FVM_ExecutionDragBounds *bounds);
    /* Apply the current pointer through parent transforms, clamp and atomically update position.
     * Removed targets stop dragging. Reentrant update is invalid; superseded work is discarded. */
    FVM_API FVM_Result fvm_exec_drag_update(FVM_Context *);
    /* Flush pending motion then end the context-wide drag. Inside an update, cancel immediately.
     * Update errors preserve active state. Callback side effects are not rolled back. */
    FVM_API FVM_Result fvm_exec_drag_end(FVM_Context *);
    /* Read the active target as an object value or undefined, without allocation or callbacks. */
    FVM_API FVM_Result fvm_exec_drag_target(FVM_Context *, FVM_Value *out);

    /* Snapshot the context pointer in a native clip's local coordinates using its world matrix.
     * Outputs must be non-NULL and distinct; errors preserve both outputs. Loaded SWFs use
     * binary32 device-pixel rounding and identity fallback for singular projection. Unloaded
     * hosts retain strict finite double projection. No bytecode or replaceable mouse property is used.
     * [GC:] Retains the clip and blocks collection while transform getters can execute script. */
    FVM_API FVM_Result fvm_exec_clip_pointer(FVM_Context *, FVM_Object *clip,
                                              FVM_Value *local_x, FVM_Value *local_y);

    /* Coerce absolute display depth, then name, then resolve source from an explicit start.
     * Duplicate natively without script method lookup. Absolute depth differs from public
     * depth by the native display bias. Missing sources/invalid Flash depths yield undefined;
     * depths beyond the implemented native range return unsupported. Errors preserve out.
     * [GC:] Retains all operands through collecting coercions and native construction. */
    FVM_API FVM_Result fvm_exec_duplicate_target(FVM_Context *, FVM_Object *start,
                                                  const FVM_Value *source, const FVM_Value *name,
                                                  const FVM_Value *depth, FVM_Value *out);

    /* Duplicate a native sibling with explicit name/depth and optional initialization value.
     * Uses the native public-depth range and existing instance construction policy, without
     * looking up replaceable script methods. A root has no sibling and returns undefined.
     * Errors preserve out; constructor errors can follow scene publication. Aliases are allowed.
     * [GC:] Retains all explicit operands; native construction blocks collector reentry. */
    FVM_API FVM_Result fvm_exec_duplicate_clip(FVM_Context *, FVM_Object *clip,
                                                const FVM_Value *name, const FVM_Value *depth,
                                                const FVM_Value *initial, FVM_Value *out);

    /* Intrinsic world-space native geometry, independent of replaceable script methods.
     * Object form compares stroked world bounds; point form selects bounds or painted shape.
     * Empty geometry/nonfinite points yield false. Errors preserve the Boolean output.
     * [GC:] Retains clips and blocks collector reentry during native geometry callbacks. */
    FVM_API FVM_Result fvm_exec_clip_hit_object(FVM_Context *, FVM_Object *clip,
                                                FVM_Object *target, FVM_Value *out);
    FVM_API FVM_Result fvm_exec_clip_hit_point(FVM_Context *, FVM_Object *clip,
                                               double world_x, double world_y, int shape,
                                               FVM_Value *out);

    /* Resolve an explicit native clip or versioned path, without changing activation targets.
     * Empty paths resolve to start only when allow_empty is nonzero. Missing/nonclip targets
     * produce undefined. Output may alias input; errors preserve output.
     * [GC:] Retains start/input/path through script conversion and path callbacks. */
    FVM_API FVM_Result fvm_exec_resolve_clip(FVM_Context *, FVM_Object *start,
                                            const FVM_Value *input, int allow_empty,
                                            FVM_Value *out);

    /* Resolve a native clip value or versioned target path from an explicit start, then apply
     * dynamic removal policy. Does not change activation targets or deliver unload events.
     * [GC:] Retains start/input/conversion through path callbacks and removal preparation. */
    FVM_API FVM_Result fvm_exec_remove_target(FVM_Context *, FVM_Object *start,
                                               const FVM_Value *target);


    /* Prepare a frame/label without seeking: nonnumbers stringify once; complete decimal digits
     * become numbers. with_bias adds scene_bias to numbers only, including zero bias when enabled.
     * Context supplies versioned coercion; no Action flags or bytecode are accepted.
     * [GC:] Retains input/conversion; aliases are allowed and failure leaves output intact. */
    FVM_API FVM_Result fvm_exec_frame_request(FVM_Context *, const FVM_Value *, int with_bias,
                                               unsigned long scene_bias, FVM_Value *);
    /* Execute a built-in frame command on an explicit clip. */
    FVM_API FVM_Result fvm_exec_frame_control(FVM_Context *, FVM_Object *, FVM_ExecutionFrame,
                                              const FVM_Value *frame, int playing);
    /* Call frame actions, including clip:frame paths, without seeking the timeline. */
    FVM_API FVM_Result fvm_exec_call_frame(FVM_Context *, FVM_Object *scope_receiver,
                                           const FVM_Value *frame);
/* [GC:] Create a retained object scope; null/undefined keep the parent unchanged.
     * Primitive values are boxed. Callers retain the returned scope across collections. */
    FVM_API FVM_Result fvm_exec_with_scope(FVM_Context *, FVM_Object *parent,
                                           const FVM_Value *, FVM_Object **scope);
    /* Resolve a counted variable name in a lexical chain. found distinguishes an
     * undefined binding from absence; owner identifies its object for assignment/calls. */
    FVM_API FVM_Result fvm_exec_scope_lookup(FVM_Context *, FVM_Object *scope,
                                             const FVM_Value *name, FVM_Value *result,
                                             FVM_Object **owner, int *found);
    /* Resolve a lexical binding and capture whether it came from With before
     * its getter runs. Existing scope_lookup remains the compatible facade.
     * [GC:] Retains scope/name through getters; errors preserve all outputs. */
    FVM_API FVM_Result fvm_exec_scope_resolve(FVM_Context *, FVM_Object *scope,
                                              const FVM_Value *name, FVM_Value *result,
                                              FVM_Object **owner, int *found, int *with_scope);
    /* Delete a simple binding in activation/With scopes, then target and global.
     * Presence checks never invoke getters; an inherited With/target binding blocks
     * deletion of an outer binding. Qualified path syntax belongs to the caller.
     * [GC:] Inputs are retained through name coercion; errors preserve out. */
    FVM_API FVM_Result fvm_exec_scope_delete(FVM_Context *, FVM_Object *scope,
                                             FVM_Object *target, const FVM_Value *name,
                                             FVM_Value *out);
    /* Delete a member through a counted dot/colon path rooted at base. Prefix getters
     * may run; the final member is not read. Primitive/missing prefixes report false.
     * Slash prefixes use display-path resolution relative to base; a slash in the final key
     * is literal. This service does not search outer lexical scopes.
     * [GC:] Traversal values and text are retained; errors preserve out. */
    FVM_API FVM_Result fvm_exec_delete_path(FVM_Context *, const FVM_Value *base,
                                            const FVM_Value *path, FVM_Value *out);
    /* Resolve an object path by retrying each lexical scope, selected target and global.
     * Absolute/_root paths use the target's display root. Missing paths return undefined.
     * receiver supplies SWF5+ this independently of target; null keeps ordinary lookup.
     * This service resolves objects, not a final variable.
     * [GC:] Retains scopes, target and counted path; errors preserve out. */
    FVM_API FVM_Result fvm_exec_scope_target(FVM_Context *, FVM_Object *scope,
                                             FVM_Object *target, FVM_Object *receiver,
                                             const FVM_Value *path,
                                             FVM_Value *out);
    /* Read a counted member on paths retried across scopes/target/global. present
     * distinguishes absence from a final undefined/null/primitive value.
     * [GC:] Retains operands across getters; errors preserve out and present. */
    FVM_API FVM_Result fvm_exec_scope_member(FVM_Context *, FVM_Object *scope,
                                             FVM_Object *target, FVM_Object *receiver,
                                             const FVM_Value *path, const FVM_Value *name,
                                             FVM_Value *out, int *present);
    /* Delete a scope binding or qualified reference under loaded-version path rules.
     * [GC:] Coerces/root-retains the name and owner; no bytecode or stack is accepted. */
    FVM_API FVM_Result fvm_exec_delete_reference(FVM_Context *, FVM_Object *scope,
                                                 FVM_Object *target, FVM_Object *receiver,
                                                 const FVM_Value *name, FVM_Value *out);
    /* Resolve the versioned qualified/slash phase of a counted variable reference.
     * handled=0 requests simple binding lookup and leaves out untouched; present
     * distinguishes a handled missing path from a present undefined value.
     * [GC:] Retains inputs and allows out/name aliasing; errors preserve all outputs. */
    FVM_API FVM_Result fvm_exec_variable_path(FVM_Context *, FVM_Object *scope,
                                              FVM_Object *target, FVM_Object *receiver,
                                              const FVM_Value *name, FVM_Value *out,
                                              int *handled, int *present);
    /* Assign a counted reference; simple bindings use getter-free presence checks,
     * qualified owners retry scoped paths, and empty names are ignored. Bare slash
     * names are literal bindings. Null target selects global fallback.
     * [GC:] Retains name/value, scope, destination and this through getter/setter callbacks. */
    /* Loaded SWFs coerce names once before lookup; host-only calls require a String.
     * Inputs and selected scope objects remain GC roots through conversion/setters. */
    FVM_API FVM_Result fvm_exec_variable_set(FVM_Context *, FVM_Object *scope,
                                             FVM_Object *target, FVM_Object *receiver,
                                             const FVM_Value *name, const FVM_Value *value);
    /* Read a plain binding and select its callable receiver. local_scope controls
     * historical implicit-this casing; null default_receiver uses the global object.
     * With bindings use their wrapped receiver, activation/global bindings keep the
     * supplied default, and target bindings use target. Qualified paths are separate.
     * [GC:] Retains explicit objects/name/result; errors preserve all outputs. */
    FVM_API FVM_Result fvm_exec_variable_lookup(FVM_Context *, FVM_Object *scope,
                                                FVM_Object *target, FVM_Object *receiver,
                                                FVM_Object *default_receiver, int local_scope,
                                                const FVM_Value *name, FVM_Value *result,
                                                FVM_Object **call_receiver, int *found);
    /* Declaration modes distinguish an absent initializer from an explicit undefined. */
#define FVM_EXEC_DECLARE_UNINITIALIZED 0 /* Preserve applicable existing declarations. */
#define FVM_EXEC_DECLARE_INITIALIZED 1 /* Declare/write the supplied value. */
    /* Declare a counted/coercible local name. activation identifies a function/local
     * frame; top-level qualified names use path assignment. Uninitialized value may
     * be null. Loaded With declarations distinguish own from inherited properties.
     * [GC:] Retains all inputs, converted text and qualified reads through callbacks. */
    FVM_API FVM_Result fvm_exec_variable_declare(FVM_Context *, FVM_Object *scope,
                                                 FVM_Object *activation, FVM_Object *target,
                                                 FVM_Object *receiver, int initialized,
                                                 const FVM_Value *name, const FVM_Value *value);
    /* Resolve/call an already-coerced counted name with ordered arguments. Loaded
     * qualified names preserve their owning receiver; missing/noncallable loaded
     * bindings return undefined and do not consult stale script records. Host mode
     * keeps literal named lookup/fallback. Null default_receiver uses global.
     * [GC:] Retains environment, name, callee, receiver and copied args through callbacks. */
    FVM_API FVM_Result fvm_exec_call_function(FVM_Context *, FVM_Object *scope, FVM_Object *target,
                                              FVM_Object *receiver, FVM_Object *default_receiver,
                                              int local_scope, const FVM_Value *name,
                                              unsigned int argc, const FVM_Value *argv,
                                              FVM_Value *result);
    /* Complete GetVariable: loaded names are string-coerced, paths/plain names
     * resolve through explicit scopes, and loaded SWF4 MovieClip values become undefined.
     * Host names must already be strings. Raw path/lookup services do not apply this filter.
     * [GC:] Retains environment, name and intermediates; failure preserves result. */
    FVM_API FVM_Result fvm_exec_get_variable(FVM_Context *, FVM_Object *scope,
                                            FVM_Object *target, FVM_Object *receiver,
                                            int local_scope, const FVM_Value *name,
                                            FVM_Value *result);
    /* Construct from an already-coerced counted name and ordered values. Loaded
     * NewObject uses literal scoped lookup and returns undefined for nonconstructors.
     * Host mode retains qualified lookup and named native-constructor fallback.
     * [GC:] Retains environment, name, constructor and copied args through callbacks. */
    FVM_API FVM_Result fvm_exec_new_object(FVM_Context *, FVM_Object *scope,
                                          FVM_Object *target, FVM_Object *receiver,
                                          int local_scope, const FVM_Value *name,
                                          unsigned int argc, const FVM_Value *argv,
                                          FVM_Value *result);
    /* Read/write a context-global value slot. Missing slots read undefined and
     * discard valid writes. Stored strings are owned; out may alias value. */
    FVM_API FVM_Result fvm_exec_global_register(FVM_Context *, int write,
                                                unsigned int index, const FVM_Value *value,
                                                FVM_Value *out);
    /* Convert to Number, or legacy signed Int32 when integer is nonzero.
     * [GC:] Retains input through valueOf callbacks; errors preserve out and aliasing is allowed. */
    FVM_API FVM_Result fvm_exec_to_number(FVM_Context *, int integer,
                                          const FVM_Value *, FVM_Value *out);
    /* Project version-aware truth without user callbacks; output may alias input. */
    FVM_API FVM_Result fvm_exec_to_boolean(FVM_Context *, const FVM_Value *, FVM_Value *out);
    /* Return the existing AVM1 type label as immutable borrowed text, without callbacks. */
    FVM_API FVM_Result fvm_exec_type_of(FVM_Context *, const FVM_Value *, FVM_Value *out);
    /* Box primitives through the global class; null/undefined yield undefined.
     * Loaded missing/nonconstructable classes yield undefined. Output may alias input. */
    FVM_API FVM_Result fvm_exec_to_object(FVM_Context *, const FVM_Value *, FVM_Value *out);
    /* Convert with loaded SWF string policy; otherwise use ordinary host conversion.
     * [GC:] Retains input through callbacks; errors preserve out and aliasing is allowed.
     * An unchanged input string remains borrowed under the ordinary value ownership contract. */
    FVM_API FVM_Result fvm_exec_to_string(FVM_Context *, const FVM_Value *, FVM_Value *out);
    /* Convert a count and cap it to available values under loaded AVM1 policy.
     * Unloaded hosts require a nonnegative integral number and sufficient values.
     * Actual counts above the method-service bound fail; errors preserve out. */
    FVM_API FVM_Result fvm_exec_argument_count(FVM_Context *, const FVM_Value *,
                                              size_t available, unsigned int *out);
    /* Dispatch a resolved named binding, or the existing named fallback when absent.
     * found distinguishes an undefined binding from a missing name. The caller supplies
     * scope resolution and receiver; [GC:] values remain retained across calls/construction.
     * Errors preserve out; no interpreter scope map or bytecode is accepted. */
    FVM_API FVM_Result fvm_exec_resolved_call(FVM_Context *, int construct,
                                              const FVM_Value *name, const FVM_Value *function,
                                              int found, FVM_Object *receiver, unsigned int argc,
                                              const FVM_Value *argv, FVM_Value *out);
    /* Invoke or construct a value member. Loaded SWFs coerce names to text;
     * undefined/empty names select an anonymous call. Unloaded host contexts
     * also treat null as anonymous and accept only string/null/undefined names.
     * anonymous_this supplies the explicit receiver. [GC:] Inputs and converted
     * text remain rooted through callbacks; failures leave out unchanged. */
    FVM_API FVM_Result fvm_exec_call_method(FVM_Context *, int construct,
                                            const FVM_Value *owner, const FVM_Value *name,
                                            FVM_Object *anonymous_this, unsigned int argc,
                                            const FVM_Value *argv, FVM_Value *out);
#ifdef __cplusplus
}
#endif
#endif
