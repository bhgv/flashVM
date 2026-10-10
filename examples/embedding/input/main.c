/* C89: configure multitap and swipe mappings. Host must recognize gestures. */
#include "flashvm/input.h"
#include <stdio.h>
int main(void) {
 FVM_Config cfg;FVM_Runtime *r=0;FVM_Context *c=0;FVM_InputRule rule;FVM_InputEvent e;FVM_MouseButton b;
 if(fvm_config_init(&cfg)!=FVM_OK || fvm_runtime_create(&cfg,&r)!=FVM_OK || fvm_context_create(r,&c)!=FVM_OK) return 1;
 rule.struct_size=sizeof(rule);rule.gesture=FVM_GESTURE_TAP;rule.fingers=2;rule.direction=0;rule.button=FVM_MOUSE_RIGHT;
 if(fvm_input_set_rule(c,&rule)!=FVM_OK) return 2;
 rule.gesture=FVM_GESTURE_SWIPE;rule.fingers=3;rule.direction=-2;rule.button=FVM_MOUSE_WHEEL_UP;
 if(fvm_input_set_rule(c,&rule)!=FVM_OK) return 3;
 e.gesture=FVM_GESTURE_SWIPE;e.fingers=3;e.direction=-2;e.x=10;e.y=20;
 if(fvm_input_map(c,&e,&b)!=FVM_OK || b!=FVM_MOUSE_WHEEL_UP) return 4;
 printf("3-finger upward swipe mapped to button %d\n",(int)b);
 fvm_context_destroy(c);fvm_runtime_destroy(r);return 0;
}
