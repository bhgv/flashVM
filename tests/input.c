#include "flashvm/input.h"
int main(void) {
 FVM_Runtime *r=0;FVM_Context *c=0;FVM_Config cfg;FVM_InputRule rule;FVM_InputEvent event;FVM_MouseButton button;
 if(fvm_config_init(&cfg)!=FVM_OK || fvm_runtime_create(&cfg,&r)!=FVM_OK || fvm_context_create(r,&c)!=FVM_OK) return 1;
 rule.struct_size=sizeof(rule);rule.gesture=FVM_GESTURE_TAP;rule.fingers=2;rule.direction=0;rule.button=FVM_MOUSE_RIGHT;
 if(fvm_input_set_rule(c,&rule)!=FVM_OK) return 2;
 event.gesture=FVM_GESTURE_TAP;event.fingers=2;event.direction=0;event.x=0;event.y=0;
 if(fvm_input_map(c,&event,&button)!=FVM_OK || button!=FVM_MOUSE_RIGHT) return 3;
 event.fingers=3;if(fvm_input_map(c,&event,&button)!=FVM_OK || button!=FVM_MOUSE_NONE) return 4;
 fvm_context_destroy(c);fvm_runtime_destroy(r);return 0;
}
