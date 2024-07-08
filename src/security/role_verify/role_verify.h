#ifndef ROLE_VERIFY_FUNC_H
#define ROLE_VERIFY_FUNC_H


// plugin's init func and kickstart func
int role_verify_init(void * sub_proc,void * para);
int role_verify_start(void * sub_proc,void * para);
struct timeval time_val={0,50*1000};

#endif
