/* Private owner-thread MovieClip chrome primitives; never a channel wire ABI. */
#ifndef DVM_DESKTOP_CHROME_H
#define DVM_DESKTOP_CHROME_H
#include "flashvm/movieclip.h"


/* Copied descriptor subset for visual decoration; no document/VM ownership. */
typedef struct DVM_ChromeItem
{
    unsigned long id;             /* Stable scene identity. */
    unsigned long dynamic;        /* Vector paths require a clean graphics list before append. */
    unsigned long active;         /* Copied selected-window state. */
    unsigned long selected;       /* Copied expanded-tool state. */
    double preview_scale;         /* Actual uniform owner-local mirror transform. */
    unsigned long preview_width;  /* Admitted shelf/card physical width. */
    unsigned long preview_height; /* Admitted shelf/card physical height. */
    unsigned long palette;        /* Copied compact popup role; appearance remains in AS. */
    unsigned long close_button;   /* Copied title close role, distinct from document tabs. */
    unsigned long mirror;         /* Zero or copied thumbnail source identity. */
} DVM_ChromeItem;


/* Clear/build a rounded owner-local control. Borrow context/clip; return0 on error. */
int dvm_chrome_box(
    FVM_Context *owner, FVM_Object *clip, double wide, double high, unsigned long rgb, unsigned long border);


/* Append a copied filled polygon. Borrow arrays for count points and owner clip. */
int dvm_chrome_patch(FVM_Context *owner,
                     FVM_Object *clip,
                     const double *x,
                     const double *y,
                     unsigned int count,
                     unsigned long rgb,
                     double alpha);


/* Build cached frosted rail graphics. Cache extents belong to the caller's context. */
int dvm_chrome_glass(FVM_Context *owner,
                     FVM_Object *clip,
                     double wide,
                     double high,
                     int right,
                     unsigned int *cached_width,
                     unsigned int *cached_height);


/* Append desktop, header and scroll decorations from copied scalar descriptors. */
int dvm_chrome_decorate(FVM_Context *owner, FVM_Object *clip, const DVM_ChromeItem *item, double wide, double high);


/* Invalidate retained graphics after a clear. Borrow owner/clip; return0 on VM error. */
int dvm_chrome_invalidate(FVM_Context *owner, FVM_Object *clip);


/* Return whether this copied role has a dedicated AS appearance module. */
int dvm_chrome_owns_view(unsigned long id, unsigned long mirror);


/* Bind a generic owner-local graphics capability; no UI layout/style is encoded. */
int dvm_chrome_drawing_bind(FVM_Context *owner, FVM_Object *clip);
/* Run the AS vector-control view with copied palette/selection, UI owner only. */
int dvm_chrome_vector(FVM_Context *owner, FVM_Object *clip, double wide, double high,
                       unsigned long color, int selected);
#endif
