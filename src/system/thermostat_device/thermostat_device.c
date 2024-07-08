#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include "json.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "basefunc.h"
#include "memdb.h"
#include "message.h"
#include "channel.h"
#include "connector.h"
#include "ex_module.h"
#include "sys_func.h"

#include "thermostat_device.h"
#include "modbus_tcp.h"
#include "modbus_state.h"
#include "modbus_cmd.h"
#include "thermostat_data.h"

#define MAX_LINE_LEN 1024

static BYTE Buf[DIGEST_SIZE*64];
int NUM = 0B00000000;

int environment_T;
int rand_offset=0;
char rand_source[DIGEST_SIZE];


int proc_read_holding_registers(void * sub_proc,void * recv_msg);
int proc_read_coils(void * sub_proc,void * recv_msg);
int proc_read_input_registers(void * sub_proc,void * recv_msg);
int proc_write_coil(void * sub_proc,void * recv_msg);
int proc_write_register(void * sub_proc,void * recv_msg);
int proc_temperature_change(void * sub_proc);

int thermostat_device_init(void * sub_proc,void * para)
{
    
    int ret;
    RECORD(THERMOSTAT_DATA,MACHINE_STATE) * machine_state;
    struct init_para * init_para=(struct init_para *)para;


    machine_state = Dalloc0(sizeof(*machine_state),sub_proc);
    if(machine_state==NULL)
        return -ENOMEM;
    machine_state->target_t = init_para->default_target;
    environment_T = init_para->environment_T;
    machine_state->current_t = environment_T;

    ex_module_setpointer(sub_proc,machine_state);
    return 0;
}

int thermostat_device_start(void * sub_proc,void * para)
{
    int ret = 0;
    int rc = 0;
    RECORD(THERMOSTAT_DATA,MACHINE_STATE) * machine_state;
    void * recv_msg;
    void * send_msg;
    int type;
    int subtype;

    for (;;)
    {
        usleep(time_val.tv_usec);
        ret=ex_module_recvmsg(sub_proc,&recv_msg); 
        if(ret<0)
            continue;
        if(recv_msg==NULL)
            continue;
        type=message_get_type(recv_msg);
        subtype=message_get_subtype(recv_msg);
        if(!memdb_find_recordtype(type,subtype))
        {
            printf("message format (%d %d) is not registered!\n",
                message_get_type(recv_msg),message_get_subtype(recv_msg));
            continue;
        }
        if((type==TYPE(MODBUS_CMD))&&(subtype==SUBTYPE(MODBUS_CMD,READ_COILS)))
        {
            //printf("client message %p\n",sub_proc);
            ret=proc_read_coils(sub_proc,recv_msg); 
        }
        else if((type==TYPE(MODBUS_CMD))&&(subtype==SUBTYPE(MODBUS_CMD,READ_HOLDING_REGISTERS)))
        {
            //printf("client message %p\n",sub_proc);
            ret=proc_read_holding_registers(sub_proc,recv_msg); 
        }
        else if((type==TYPE(MODBUS_CMD))&&(subtype==SUBTYPE(MODBUS_CMD,READ_INPUT_REGISTERS)))
        {
            //printf("client message %p\n",sub_proc);
            ret=proc_read_input_registers(sub_proc,recv_msg); 
        }
        else if((type==TYPE(MODBUS_CMD))&&(subtype==SUBTYPE(MODBUS_CMD,WRITE_SINGLE_COIL)))
        {
            ret=proc_write_coil(sub_proc,recv_msg); 
        }
        else if((type==TYPE(MODBUS_CMD))&&(subtype==SUBTYPE(MODBUS_CMD,WRITE_SINGLE_REGISTER)))
        {
            ret=proc_write_register(sub_proc,recv_msg); 
        }
        proc_temperature_change(sub_proc);
    }
    return 0;
}

int proc_read_coils(void * sub_proc,void * recv_msg)
{
    int ret = 0;
    RECORD(THERMOSTAT_DATA,MACHINE_STATE) * machine_state;

    RECORD(MODBUS_CMD,READ_COILS) * modbus_cmd;
    RECORD(MODBUS_DATA,READ_COILS) * modbus_data;

    machine_state = (RECORD(THERMOSTAT_DATA,MACHINE_STATE)*) ex_module_getpointer(sub_proc);
    if(machine_state == NULL)
        return -EINVAL;

    ret = message_get_record(recv_msg,&modbus_cmd,0);
    if(ret<0)
        return ret;
    if(modbus_cmd ==NULL)
        return -EINVAL;

    modbus_data=Talloc0(sizeof(*modbus_data));
    if(modbus_data==NULL)
        return -ENOMEM;
    modbus_data->area_bytes=1;
    modbus_data->value=Talloc0(modbus_data->area_bytes);
    if(modbus_data==NULL)
        return -ENOMEM;
    if(modbus_cmd->start_addr == 1)  // total switch
    {
        if(machine_state->machine_switch)
            modbus_data->value=0x01;
    }
    else if(modbus_cmd->start_addr == 2) // heating switch
    {
        if(machine_state->heating_switch)
            modbus_data->value=0x01<<(modbus_cmd->start_addr-1);
    }
    else
    {
        // error process
    }
    void * send_msg = message_create(TYPE_PAIR(MODBUS_DATA,READ_COILS),recv_msg);
    if(recv_msg == NULL)
        return -EINVAL;
    message_add_record(send_msg,modbus_data);
    
    ret=ex_module_sendmsg(sub_proc,send_msg);

    return ret;
}
int proc_read_holding_registers(void * sub_proc,void * recv_msg)
{
    int ret = 0;
    RECORD(THERMOSTAT_DATA,MACHINE_STATE) * machine_state;

    RECORD(MODBUS_CMD,READ_HOLDING_REGISTERS) * modbus_cmd;
    RECORD(MODBUS_DATA,READ_HOLDING_REGISTERS) * modbus_data;

    machine_state = (RECORD(THERMOSTAT_DATA,MACHINE_STATE)*) ex_module_getpointer(sub_proc);
    if(machine_state == NULL)
        return -EINVAL;

    ret = message_get_record(recv_msg,&modbus_cmd,0);
    if(ret<0)
        return ret;
    if(modbus_cmd ==NULL)
        return -EINVAL;

    modbus_data=Talloc0(sizeof(*modbus_data));
    if(modbus_data==NULL)
        return -ENOMEM;
    modbus_data->area_bytes=2;
    modbus_data->value=Talloc0(modbus_data->area_bytes);
    if(modbus_data==NULL)
        return -ENOMEM;
    if(modbus_cmd->start_addr == 40003)  // total switch
    {
        Memcpy(modbus_data->value,&(machine_state->target_t),sizeof(unsigned short));

    }
    else if(modbus_cmd->start_addr == 40005)
    {
	UINT16 gear_value = machine_state->heating_gear;     
        Memcpy(modbus_data->value,&(gear_value),sizeof(unsigned short));
        // error process
    }
    void * send_msg = message_create(TYPE_PAIR(MODBUS_DATA,READ_HOLDING_REGISTERS),recv_msg);
    if(send_msg == NULL)
        return -EINVAL;
    message_add_record(send_msg,modbus_data);
    
    ret=ex_module_sendmsg(sub_proc,send_msg);

    return ret;
}

int proc_read_input_registers(void * sub_proc,void * recv_msg)
{
    int ret = 0;
    RECORD(THERMOSTAT_DATA,MACHINE_STATE) * machine_state;

    RECORD(MODBUS_CMD,READ_INPUT_REGISTERS) * modbus_cmd;
    RECORD(MODBUS_DATA,READ_INPUT_REGISTERS) * modbus_data;

    machine_state = (RECORD(THERMOSTAT_DATA,MACHINE_STATE)*) ex_module_getpointer(sub_proc);
    if(machine_state == NULL)
        return -EINVAL;

    ret = message_get_record(recv_msg,&modbus_cmd,0);
    if(ret<0)
        return ret;
    if(modbus_cmd ==NULL)
        return -EINVAL;

    modbus_data=Talloc0(sizeof(*modbus_data));
    if(modbus_data==NULL)
        return -ENOMEM;
    modbus_data->area_bytes=2;
    modbus_data->value=Talloc0(modbus_data->area_bytes);
    if(modbus_data==NULL)
        return -ENOMEM;
    if(modbus_cmd->start_addr == 30004)  // total switch
    {
        Memcpy(modbus_data->value,&(machine_state->current_t),sizeof(unsigned short));
    }
    else
    {
        // error process
    }
    void * send_msg = message_create(TYPE_PAIR(MODBUS_DATA,READ_INPUT_REGISTERS),recv_msg);
    if(recv_msg == NULL)
        return -EINVAL;
    message_add_record(send_msg,modbus_data);
    
    ret=ex_module_sendmsg(sub_proc,send_msg);

    return ret;
}

int proc_write_coil(void * sub_proc,void * recv_msg)
{
    int ret = 0;
    RECORD(THERMOSTAT_DATA,MACHINE_STATE) * machine_state;

    RECORD(MODBUS_CMD,WRITE_SINGLE_COIL)* modbus_cmd;
    RECORD(MODBUS_DATA,WRITE_SINGLE_COIL) * modbus_data;

    machine_state = (RECORD(THERMOSTAT_DATA,MACHINE_STATE)*) ex_module_getpointer(sub_proc);
    if(machine_state == NULL)
        return -EINVAL;

    ret = message_get_record(recv_msg,&modbus_cmd,0);
    if(ret<0)
        return ret;
    if(modbus_cmd ==NULL)
        return -EINVAL;

    modbus_data=Talloc0(sizeof(*modbus_data));
    if(modbus_data==NULL)
        return -ENOMEM;
    modbus_data->start_addr = modbus_cmd->start_addr;
    modbus_data->convert_value=modbus_cmd->convert_value;

    if(modbus_cmd->start_addr == 1)  // total switch
    {
        if(modbus_cmd->convert_value)
            machine_state->machine_switch=1;
        else
            machine_state->machine_switch=0;
    }
    else if(modbus_cmd->start_addr == 2)  // total switch
    {
        if(modbus_cmd->convert_value)
            machine_state->heating_switch=1;
        else
            machine_state->heating_switch=0;
    }
    else
    {
        // error process
    }
    void * send_msg = message_create(TYPE_PAIR(MODBUS_DATA,WRITE_SINGLE_COIL),recv_msg);
    if(send_msg == NULL)
        return -EINVAL;
    message_add_record(send_msg,modbus_data);
    
    ret=ex_module_sendmsg(sub_proc,send_msg);

    return ret;
}

int proc_write_register(void * sub_proc,void * recv_msg)
{
    int ret = 0;
    RECORD(THERMOSTAT_DATA,MACHINE_STATE) * machine_state;

    RECORD(MODBUS_CMD,WRITE_SINGLE_REGISTER)* modbus_cmd;
    RECORD(MODBUS_DATA,WRITE_SINGLE_REGISTER) * modbus_data;

    machine_state = (RECORD(THERMOSTAT_DATA,MACHINE_STATE)*) ex_module_getpointer(sub_proc);
    if(machine_state == NULL)
        return -EINVAL;

    ret = message_get_record(recv_msg,&modbus_cmd,0);
    if(ret<0)
        return ret;
    if(modbus_cmd ==NULL)
        return -EINVAL;

    modbus_data=Talloc0(sizeof(*modbus_data));
    if(modbus_data==NULL)
        return -ENOMEM;
    modbus_data->start_addr = modbus_cmd->start_addr;
    modbus_data->convert_value=modbus_cmd->convert_value;

    if(modbus_cmd->start_addr == 40003)  // target temperature register
    {
        if(modbus_cmd->convert_value)
            machine_state->target_t =modbus_cmd->convert_value;
    }
    else if(modbus_cmd->start_addr == 40005)  // target temperature register
    {
        if(modbus_cmd->convert_value)
            machine_state->heating_gear = modbus_cmd->convert_value;
    }
    else
    {
        // error process
    }
    void * send_msg = message_create(TYPE_PAIR(MODBUS_DATA,WRITE_SINGLE_REGISTER),recv_msg);
    if(send_msg == NULL)
        return -EINVAL;
    message_add_record(send_msg,modbus_data);
    
    ret=ex_module_sendmsg(sub_proc,send_msg);

    return ret;
}

int proc_temperature_change(void * sub_proc)
{
    int ret = 0;
    RECORD(THERMOSTAT_DATA,MACHINE_STATE) * machine_state;
    int offset=0;

    int cold_weight = 1;
    int heat_weight = 8;
    int heating_t_base = 4500;
    int heating_t_span = 500;


    if(rand_offset==0)
        RAND_bytes(rand_source,DIGEST_SIZE);

    machine_state = (RECORD(THERMOSTAT_DATA,MACHINE_STATE)*) ex_module_getpointer(sub_proc);
    if(machine_state == NULL)
        return -EINVAL;

   if(machine_state->heating_switch)
   {
        machine_state->current_t += cold_weight *(environment_T-machine_state->current_t)/1000 + 
            heat_weight *(heating_t_base+heating_t_span*machine_state->heating_gear-machine_state->current_t)/1000;
        machine_state->current_t +=  rand_source[rand_offset++]/20;
   } 
   else
   {
        machine_state->current_t += cold_weight *(environment_T-machine_state->current_t)/1000; 
        machine_state->current_t += rand_source[rand_offset++]/20;
   }
   if(rand_offset == DIGEST_SIZE)
            rand_offset=0;
    return 0;
}
