/* Native CallMethod cannot create raw Array storage on a super facade. */
#define FVM_ARRAY_SUPER_JIT
#include "array_borrowed_super.cpp"
