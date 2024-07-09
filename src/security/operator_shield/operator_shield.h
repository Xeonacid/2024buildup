#ifndef OPERATOR_SHIELD_FUNC_H
#define OPERATOR_SHIELD_FUNC_H


// plugin's init func and kickstart func
int operator_shield_init(void * sub_proc,void * para);
int operator_shield_start(void * sub_proc,void * para);
struct timeval time_val={0,50*1000};

#endif
