#ifndef THERMOSTAT_DEVICE_FUNC_H
#define THERMOSTAT_DEVICE_FUNC_H

int thermostat_device_init(void * sub_proc,void * para);
int thermostat_device_start(void * sub_proc,void * para);

struct init_para
{
	UINT16 default_target;	
	UINT16 environment_T;
}__attribute__((packed));

#endif
