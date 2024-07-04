#ifndef MODBUS_ACL_FUNC_H
#define MODBUS_ACL_FUNC_H


// plugin's init func and kickstart func
int modbus_acl_init(void * sub_proc,void * para);
int modbus_acl_start(void * sub_proc,void * para);
struct timeval time_val={0,50*1000};

#endif
