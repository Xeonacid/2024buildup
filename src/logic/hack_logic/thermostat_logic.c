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

#include "thermostat_logic.h"
#include "modbus_tcp.h"
#include "modbus_state.h"
#include "modbus_cmd.h"
#include "thermostat_data.h"

#define MAX_LINE_LEN 1024

static BYTE Buf[DIGEST_SIZE*64];
struct tcloud_connector * client_conn;
int NUM = 0B00000000;

int environment_T;

char * value_file = "value_file.txt";

static int start_time;


int proc_read_target_t_cmd(void * sub_proc);
int proc_read_current_t_cmd(void * sub_proc);
int proc_read_target_t(void * sub_proc,void * recv_msg);
int proc_read_current_t(void * sub_proc,void * recv_msg);
int proc_send_switch_cmd(void * sub_proc);
int proc_send_gear_cmd(void * sub_proc);

int thermostat_logic_init(void * sub_proc,void * para)
{
    
    int ret;
    RECORD(THERMOSTAT_DATA,MACHINE_STATE) * machine_state;
    machine_state = Dalloc0(sizeof(*machine_state),sub_proc);
    if(machine_state==NULL)
        return -ENOMEM;

    ex_module_setpointer(sub_proc,machine_state);

    return 0;
}

int thermostat_logic_start(void * sub_proc,void * para)
{
    int ret = 0;
    int rc = 0;
    RECORD(THERMOSTAT_DATA,MACHINE_STATE) * machine_state;
    void * recv_msg;
    void * send_msg;
    int type;
    int subtype;
    int counter=0;
    time_t curr_time;

    int state=0;  // 1: read target state
                  // 2: read current t state
                  // 3: send heating on/off command
                  // 4: send heating gear setting command
    int offset=0;

    // 获取初始时间

    start_time = time(&curr_time);
    print_cubeaudit("start time %d\n",start_time);


    for (;;)
    {
        usleep(time_val.tv_usec);
        counter++;

        if(counter % 1000 == 1)
        {
            ret = proc_read_target_t_cmd(sub_proc);
        }

        if(counter % 100 == 2) 
        {
            state=2;    
            ret =  proc_read_current_t_cmd(sub_proc);
        }
        if(state==3)
        {
            ret = proc_send_switch_cmd(sub_proc);
        }
        if(state==4)
        {
            ret = proc_send_gear_cmd(sub_proc);
        }
        
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
        if((type==TYPE(MODBUS_DATA))&&(subtype==SUBTYPE(MODBUS_DATA,READ_HOLDING_REGISTERS)))
        {
            //printf("client message %p\n",sub_proc);
            ret=proc_read_target_t(sub_proc,recv_msg); 
            if(ret>0)
                printf(" target T is %d!\n",ret);
        }
        else if((type==TYPE(MODBUS_DATA))&&(subtype==SUBTYPE(MODBUS_DATA,READ_INPUT_REGISTERS)))
        {
            //printf("client message %p\n",sub_proc);

            ret=proc_read_current_t(sub_proc,recv_msg); 
            int fd = open(value_file,O_WRONLY);
            if( fd >0)
            {
                dprintf (fd,"%5d ",ret);
                offset+=6;
                if(offset>72)
                {
                    dprintf(fd,"\n");
                    offset=0;
                }
            }
            close(fd);
            state=3;
        }
        else if((type==TYPE(MODBUS_DATA))&&(subtype==SUBTYPE(MODBUS_DATA,WRITE_SINGLE_COIL)))
        {
            state=4;

        }
        else if((type==TYPE(MODBUS_DATA))&&(subtype==SUBTYPE(MODBUS_DATA,WRITE_SINGLE_REGISTER)))
        {
            state=0;
        }
    }
    return 0;
}


int proc_read_target_t_cmd(void * sub_proc)
{
    int ret;
    RECORD(MODBUS_CMD,READ_HOLDING_REGISTERS) * read_cmd;
    void * send_msg;
        
    read_cmd = Talloc0(sizeof(*read_cmd));
    if(read_cmd == NULL)
        return -ENOMEM;
   read_cmd->start_addr = 40000+3;
   read_cmd->reg_num=1;
   
   send_msg=message_create(TYPE_PAIR(MODBUS_CMD,READ_HOLDING_REGISTERS),NULL);
   if(send_msg==NULL)
        return -EINVAL;
   ret=message_add_record(send_msg,read_cmd);
   ret=ex_module_sendmsg(sub_proc,send_msg);
    return ret;
}

int proc_read_current_t_cmd(void * sub_proc)
{
    int ret;
    RECORD(MODBUS_CMD,READ_INPUT_REGISTERS) * read_cmd;
    void * send_msg;
        
    read_cmd = Talloc0(sizeof(*read_cmd));
    if(read_cmd == NULL)
        return -ENOMEM;
   read_cmd->start_addr = 30000+4;
   read_cmd->reg_num=1;
   
   send_msg=message_create(TYPE_PAIR(MODBUS_CMD,READ_INPUT_REGISTERS),NULL);
   if(send_msg==NULL)
        return -EINVAL;
   ret=message_add_record(send_msg,read_cmd);
   ret=ex_module_sendmsg(sub_proc,send_msg);
    return ret;
}
int proc_read_target_t(void * sub_proc,void * recv_msg)
{

    int ret;
    RECORD(THERMOSTAT_DATA,MACHINE_STATE) * machine_state;

    RECORD(MODBUS_DATA,READ_HOLDING_REGISTERS) * read_data;
    ret = message_get_record(recv_msg,&read_data,0);
    if(ret<0)
           return ret;
    if(read_data ==NULL)
           return -EINVAL;
    machine_state=ex_module_getpointer(sub_proc);
    if(machine_state->target_t == *(unsigned short *)(read_data->value))
            return 0;
    machine_state->target_t = *(unsigned short *)(read_data->value); 
    
    return machine_state->target_t;
}
int proc_read_current_t(void * sub_proc,void * recv_msg)
{
    int ret;

    RECORD(THERMOSTAT_DATA,MACHINE_STATE) * machine_state;

    RECORD(MODBUS_DATA,READ_INPUT_REGISTERS) * read_data;
    ret = message_get_record(recv_msg,&read_data,0);
    if(ret<0)
           return ret;
    if(read_data ==NULL)
           return -EINVAL;
    machine_state=ex_module_getpointer(sub_proc);
    machine_state->current_t = *(unsigned short *)(read_data->value); 
    
    return machine_state->current_t;
}

int proc_send_switch_cmd(void * sub_proc)
{
    int ret=0;
    RECORD(THERMOSTAT_DATA,MACHINE_STATE) * machine_state;
    RECORD(MODBUS_CMD,WRITE_SINGLE_COIL) * switch_cmd;
    machine_state=ex_module_getpointer(sub_proc);
        
    switch_cmd=Talloc0(sizeof(*switch_cmd));
    if(switch_cmd == NULL)
        return -ENOMEM;

    time_t curr_time;
    int comp_time;
    comp_time=time(&curr_time);


    if(machine_state->target_t<=machine_state->current_t)
    {
        switch_cmd->start_addr=2;
        switch_cmd->convert_value=0;
    }
    else
    {
        switch_cmd->start_addr=2;
        switch_cmd->convert_value=0xff00;
    }
    if(comp_time - start_time > 10)
    {
        switch_cmd->convert_value=0xff00;
	    
    }

    void * send_msg = message_create(TYPE_PAIR(MODBUS_CMD,WRITE_SINGLE_COIL),NULL);
    if(send_msg==NULL)
        return -EINVAL;

    ret = message_add_record(send_msg,switch_cmd);
    if(ret<0)
           return ret;
    ret = ex_module_sendmsg(sub_proc,send_msg);
    
    return ret;
}

int proc_send_gear_cmd(void * sub_proc)
{
    int ret=0;

    time_t curr_time;

    int comp_time;

    RECORD(THERMOSTAT_DATA,MACHINE_STATE) * machine_state;
    RECORD(MODBUS_CMD,WRITE_SINGLE_REGISTER) * gear_cmd;
    machine_state=ex_module_getpointer(sub_proc);
        
    gear_cmd=Talloc0(sizeof(*gear_cmd));
    if(gear_cmd == NULL)
        return -ENOMEM;

   int T_diff =machine_state->target_t-machine_state->current_t;

    gear_cmd->start_addr=40005;

    comp_time=time(&curr_time);

    if(comp_time - start_time > 10)
    {
	    gear_cmd->convert_value=9;
	    
    }
    else if(T_diff>10)
    {
        gear_cmd->convert_value = T_diff/100+1;
        if(gear_cmd->convert_value>9)
            gear_cmd->convert_value=9;
    }

    void * send_msg = message_create(TYPE_PAIR(MODBUS_CMD,WRITE_SINGLE_REGISTER),NULL);
    if(send_msg==NULL)
        return -EINVAL;

    ret = message_add_record(send_msg,gear_cmd);
    if(ret<0)
        return ret;
    ret = ex_module_sendmsg(sub_proc,send_msg);
    return ret;
}   
