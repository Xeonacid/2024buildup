#ifndef MODBUS_ZUC_CRYPT_FUNC_H
#define MODBUS_ZUC_CRYPT_FUNC_H

int modbus_zuc_crypt_init(void * sub_proc,void * para);
int modbus_zuc_crypt_start(void * sub_proc,void * para);
struct zuc_init_struct
{
	int  mode;   // 0 means server(slave), 1 means client(master);
};

#endif
