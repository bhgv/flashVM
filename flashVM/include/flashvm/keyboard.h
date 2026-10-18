/* FlashVM: owning-thread keyboard snapshots and the explicit AS1/AS2 Key polling bridge. */
#ifndef FLASHVM_KEYBOARD_H
#define FLASHVM_KEYBOARD_H /* Guard the public C89 keyboard ABI. */
#include "flashvm.h"
#define FVM_SELECTION_MIN_SWF_VERSION 6U /* First supported MovieClip Selection contract. */
#define FVM_KEYBOARD_MAX_LISTENERS 1024U /* Bounded unique listener objects per context. */
#define FVM_KEYBOARD_FIRST_CODE 1U       /* Zero means no key and cannot be held. */
#define FVM_KEYBOARD_CODE_COUNT 256U     /* Virtual-key slots, indexed from zero through 255. */
#define FVM_KEYBOARD_ASCII_MAX 127U /* Highest accepted ASCII character value; IME is separate. */
#define FVM_KEY_BACKSPACE 8U        /* Backspace virtual key. */
#define FVM_KEY_TAB 9U              /* Tab virtual key. */
#define FVM_KEY_ENTER 13U           /* Return/Enter virtual key. */
#define FVM_KEY_SHIFT 16U           /* Either Shift key. */
#define FVM_KEY_CONTROL 17U         /* Either Control key. */
#define FVM_KEY_ALT 18U             /* Either Alt key; no Key.ALT constant is installed. */
#define FVM_KEY_CAPSLOCK 20U        /* Caps Lock virtual key and toggle selector. */
#define FVM_KEY_ESCAPE 27U          /* Escape virtual key. */
#define FVM_KEY_SPACE 32U           /* Space virtual key. */
#define FVM_KEY_PGUP 33U            /* Page Up virtual key. */
#define FVM_KEY_PGDN 34U            /* Page Down virtual key. */
#define FVM_KEY_END 35U             /* End virtual key. */
#define FVM_KEY_HOME 36U            /* Home virtual key. */
#define FVM_KEY_LEFT 37U            /* Left arrow virtual key. */
#define FVM_KEY_UP 38U              /* Up arrow virtual key. */
#define FVM_KEY_RIGHT 39U           /* Right arrow virtual key. */
#define FVM_KEY_DOWN 40U            /* Down arrow virtual key. */
#define FVM_KEY_INSERT 45U          /* Insert virtual key. */
#define FVM_KEY_DELETE 46U          /* Delete virtual key. */
#define FVM_KEY_DIGIT0 48U          /* Start of the contiguous digit virtual-key range. */
#define FVM_KEY_LETTER_A 65U      /* Start of the contiguous uppercase letter virtual-key range. */
#define FVM_KEY_NUMPAD0 96U       /* Start of the contiguous numeric-keypad virtual-key range. */
#define FVM_KEY_F1 112U           /* Start of the contiguous F1 through F12 virtual-key range. */
#define FVM_KEY_SEMICOLON 186U    /* OEM semicolon/colon key for the English mapping. */
#define FVM_KEY_EQUALS 187U       /* OEM equals/plus key. */
#define FVM_KEY_COMMA 188U        /* OEM comma/less-than key. */
#define FVM_KEY_MINUS 189U        /* OEM minus/underscore key. */
#define FVM_KEY_PERIOD 190U       /* OEM period/greater-than key. */
#define FVM_KEY_SLASH 191U        /* OEM slash/question key. */
#define FVM_KEY_BACKQUOTE 192U    /* OEM grave/tilde key. */
#define FVM_KEY_LEFTBRACKET 219U  /* OEM left bracket/brace key. */
#define FVM_KEY_BACKSLASH 220U    /* OEM backslash/bar key. */
#define FVM_KEY_RIGHTBRACKET 221U /* OEM right bracket/brace key. */
#define FVM_KEY_QUOTE 222U        /* OEM quote/double-quote key. */
#define FVM_KEY_NUMLOCK 144U      /* Num Lock toggle selector. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* Copied native state; no managed pointers, callbacks, OS handles or text composition. */
    typedef struct FVM_KeyboardState
    {
        unsigned long struct_size; /* Exact sizeof(FVM_KeyboardState), required on input/output. */
        unsigned char down[FVM_KEYBOARD_CODE_COUNT]; /* Each slot must be zero or one. */
        unsigned int last_code; /* Most recent key-down virtual code; zero before input/reset. */
        unsigned int
            last_ascii; /* Most recent transition's English character code, zero if absent. */
        int caps_lock;  /* Boolean Caps Lock toggle, independent of physical key-down state. */
        int num_lock;   /* Boolean Num Lock toggle, independent of physical key-down state. */
    } FVM_KeyboardState;
    /* Validate all fields before copying the snapshot; no allocation or event delivery. */
    FVM_API FVM_Result fvm_input_set_keyboard(FVM_Context *, const FVM_KeyboardState *);
    /* Copy current state; require exact struct_size and preserve output on invalid input. */
    FVM_API FVM_Result fvm_input_get_keyboard(FVM_Context *, FVM_KeyboardState *);
    /* Clear held keys, latest codes and toggles; use on focus loss before accepting new input. */
    FVM_API FVM_Result fvm_input_reset_keyboard(FVM_Context *);
    /* [GC:] Retain a same-context listener once; duplicate registration succeeds unchanged. */
    FVM_API FVM_Result fvm_input_add_key_listener(FVM_Context *, FVM_Object *);
    /* Remove by identity; require removed, set it to 0/1 on success and preserve it on error. */
    FVM_API FVM_Result fvm_input_remove_key_listener(FVM_Context *, FVM_Object *, int *removed);
    /* [GC:] Select a same-context native MovieClip or null; no script eligibility policy/events.
     * Removed clips/prototypes are rejected. Success roots the selection until clear/replacement. */
    FVM_API FVM_Result fvm_input_set_keyboard_focus(FVM_Context *, FVM_Object *clip);
    /* Return a borrowed selection; clear and return null if it or its native ancestor was removed.
     * Errors preserve output. Query may validate native storage but executes no script. */
    FVM_API FVM_Result fvm_input_get_keyboard_focus(FVM_Context *, FVM_Object **out);
    /* Publish a validated snapshot and notify the selected clip, then rooted listeners.
     * pressed must be 0 or 1. Preparation failure preserves state; callback errors stop delivery
     * after publication. Nested dispatch is rejected; listener edits affect later events. */
    FVM_API FVM_Result fvm_input_dispatch_keyboard(FVM_Context *, const FVM_KeyboardState *,
                                                   int pressed);
    /* Install explicit Selection.getFocus/setFocus for native MovieClips on SWF6--8;
     * requires standard objects, preserves conflicts, and does not install Key. */
    FVM_API FVM_Result fvm_standard_selection_install(FVM_Context *);
    /* Explicitly install the supported Key polling namespace, preserving conflicting bindings. */
    FVM_API FVM_Result fvm_standard_keyboard_install(FVM_Context *);
#ifdef __cplusplus
}
#endif
#endif
