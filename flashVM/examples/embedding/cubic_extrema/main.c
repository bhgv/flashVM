/* C89 native path tutorial; README.md and README.ru.md describe each API and owner. */
#include "flashvm/path.h" /* Own an editable native path without creating a VM. */
#include "example_constants.h" /* Select reproducible numeric bounds for this demonstration. */
#include <math.h> /* Compute the independent analytic extrema and binary scale. */
#include <stdio.h> /* Report normal success or an ordinary recoverable failure. */
#include <string.h> /* Initialize every field of each native command. */
/* Build a symmetric cubic, check ordinary bounds, then measure its tiny transformed copy. */
int main(void) /* Return zero only after both measurements pass. */
{ /* [GC:] Begin native ownership; no managed objects or collector roots are needed. */
    FVM_Path *path = 0; /* Own the path after a successful create until cleanup. */
    FVM_PathCommand command; /* Lend one initialized command; add copies its contents. */
    FVM_PathBounds bounds; /* Own the four scalar result coordinates. */
    FVM_Affine matrix; /* Lend a complete transform; the path retains no matrix pointer. */
    double scale = ldexp(1.0, FVM_CUBIC_EXAMPLE_EXPONENT); /* Preserve a nonzero tiny length. */
    double maximum = 2.0 * sqrt(3.0); /* Derive the two extrema of controls 0,12,-12,0. */
    int status = 1; /* Keep failure as the default until all checks complete. */
    if (fvm_path_create(&path) != FVM_OK) /* Acquire the native owner before editing it. */
        goto cleanup; /* Release only an owner that was actually acquired. */
    memset(&command, 0, sizeof(command)); /* Initialize active and inactive coordinate slots. */
    command.verb = FVM_PATH_MOVE; /* Start the contour at the initialized origin. */
    if (fvm_path_add(path, &command) != FVM_OK) /* Copy the move into the native contour. */
        goto cleanup; /* Handle allocation or validation errors without assertions. */
    command.verb = FVM_PATH_CUBIC; /* Select two controls followed by the endpoint. */
    command.y[0] = 12.0; /* Place the first vertical control above the origin. */
    command.y[1] = -12.0; /* Place the second control symmetrically below it. */
    if (fvm_path_add(path, &command) != FVM_OK) /* Copy all three coordinate pairs. */
        goto cleanup; /* Retain the path owner for ordinary cleanup on failure. */
    if (fvm_path_bounds(path, &bounds) != FVM_OK) /* Borrow the immutable path for measurement. */
        goto cleanup; /* A failed measurement does not transfer any ownership. */
    if (fabs(bounds.y_min + maximum) > FVM_CUBIC_EXAMPLE_TOLERANCE || /* Check the lower extremum. */
        fabs(bounds.y_max - maximum) > FVM_CUBIC_EXAMPLE_TOLERANCE) /* Check the upper extremum. */
        goto cleanup; /* Refuse to report success for an incorrect ordinary curve. */
    matrix.xx = matrix.yy = scale; /* Uniformly scale both axes without changing their signs. */
    matrix.xy = matrix.yx = matrix.x0 = matrix.y0 = 0.0; /* Disable shear and translation. */
    if (fvm_path_transform(path, &matrix) != FVM_OK) /* Apply a transactional native edit. */
        goto cleanup; /* A rejected edit leaves the previous path valid for destruction. */
    if (fvm_path_bounds(path, &bounds) != FVM_OK) /* Recompute rather than reusing old bounds. */
        goto cleanup; /* Preserve the same owner through both independent measurements. */
    if (bounds.x_min != 0.0 || bounds.x_max != 0.0 || /* The entire curve stays on the Y axis. */
        fabs(bounds.y_min / scale + maximum) > FVM_CUBIC_EXAMPLE_TOLERANCE || /* Normalize first. */
        fabs(bounds.y_max / scale - maximum) > FVM_CUBIC_EXAMPLE_TOLERANCE) /* Avoid a tiny absolute tolerance. */
        goto cleanup; /* Surface lost extrema as a normal executable-example failure. */
    puts("cubic extrema verified"); /* Print the documented success marker. */
    status = 0; /* Commit success only after the tiny curve retains both extrema. */
cleanup: /* Join all ordinary failures and the successful ownership path. */
    if (status) /* Avoid an error message on normal completion. */
        fputs("cubic extrema check failed\n", stderr); /* Explain the nonzero process result. */
    fvm_path_destroy(path); /* [GC:] Release native storage without a collector; null is safe. */
    return status; /* Return the recorded result after releasing the path. */
} /* Finish the complete C89 tutorial. */
