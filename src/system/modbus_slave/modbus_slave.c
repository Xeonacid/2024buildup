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
#include "sm3_ext.h"

#include "modbus_slave.h"
#include "modbus_tcp.h"
#include "modbus_state.h"
#include "modbus_cmd.h"

#define MAX_LINE_LEN 1024

static BYTE Buf[DIGEST_SIZE*64];
struct tcloud_connector * client_conn;
int NUM = 0B00000000;

RECORD(MODBUS_STATE,SLAVE) * slave_index;

int proc_slavestate_set(void * sub_proc,void * recv_msg);

int modbus_slave_init(void * sub_proc,void * para)
{
    
    int ret;
    struct init_para * init_para=(struct init_para *)para;


    slave_index = Dalloc0(sizeof(*slave_index),sub_proc);
    if(slave_index==NULL)
        return -ENOMEM;
    Strncpy(slave_index->slave_name,init_para->slave_name,DIGEST_SIZE);

    slave_index->unit_addr=init_para->unit_addr;
    Memcpy(slave_index->slave_key,init_para->slave_key,DIGEST_SIZE);
    ex_module_setpointer(sub_proc,slave_index);
    return 0;
}

int modbus_slave_start(void * sub_proc,void * para)
{
    int ret = 0;
    int rc = 0;
    RECORD(MODBUS_STATE,SLAVE) * slave_index;
    void * recv_msg;
    void * send_msg;
    int type;
    int subtype;

    ret = proc_slavestate_send(sub_proc);
    if(ret<0)
        return ret;
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
        if((type==TYPE(MODBUS_STATE))&&(subtype==SUBTYPE(MODBUS_STATE,SERVER)))
        {
            //printf("client message %p\n",sub_proc);
            ret=proc_slavestate_set(sub_proc,recv_msg); 
            slave_index =  ex_module_getpointer(sub_proc);
        }
        else if((type==TYPE(MODBUS_CMD)))
        {
            slave_index =  ex_module_getpointer(sub_proc);
            if(slave_index->state_machine == MODBUS_CONNECT)
            {
                slave_index->state_machine = MODBUS_RESPONSE;

            }
            else
	        {
                slave_index->state_machine=MODBUS_CONNECT;
	        }
            ret=ex_module_sendmsg(sub_proc,recv_msg);
        }
        else if((type==TYPE(MODBUS_DATA)))
        {
            slave_index =  ex_module_getpointer(sub_proc);
            if(slave_index->state_machine == MODBUS_RESPONSE)
            {
                slave_index->state_machine = MODBUS_CONNECT;
            }
            else
	        {
                slave_index->state_machine=MODBUS_CONNECT;
	        }
            ret=proc_slave_data(sub_proc,recv_msg);
        }
        else if((type==TYPE(MODBUS_TCP))&& (subtype  == SUBTYPE(MODBUS_TCP,DATAGRAM)))
        {
            proc_slave_cmd(sub_proc,recv_msg);
        }
    }
    return 0;
}

int proc_slavestate_send(void * sub_proc)
{
    int ret = 0;
    RECORD(MODBUS_STATE,SLAVE) * slave_index;
    void * send_msg;
    slave_index = (RECORD(MODBUS_STATE,SLAVE)*) ex_module_getpointer(sub_proc);

    if(slave_index == NULL)
        return -EINVAL;
    send_msg = message_create(TYPE_PAIR(MODBUS_STATE,SLAVE),NULL);
    if(send_msg==NULL)
        return -EINVAL;
    ret=message_add_record(send_msg,slave_index);
    if(ret<0)
        return -EINVAL;
    ret=ex_module_sendmsg(sub_proc,send_msg);
    return ret;
}
int proc_slavestate_set(void * sub_proc,void * recv_msg)
{
    int ret = 0;
    RECORD(MODBUS_STATE,SLAVE) * slave_index;
    RECORD(MODBUS_STATE,SERVER) * server_index;

    slave_index = (RECORD(MODBUS_STATE,SLAVE)*) ex_module_getpointer(sub_proc);
    if(slave_index == NULL)
        return -EINVAL;

    ret = message_get_record(recv_msg,&server_index,0);
    if(ret<0)
        return ret;
    if(server_index ==NULL)
        return -EINVAL;
    slave_index->state_machine=MODBUS_CONNECT;
    ret = ex_module_setpointer(sub_proc,slave_index);
    return ret;
}
int proc_slave_cmd(void * sub_proc,void * recv_msg)
{
    int ret = 0;
    RECORD(MODBUS_STATE,SERVER) * server_index;
    RECORD(MODBUS_TCP,MBAP) * modbus_mbap;
    RECORD(MODBUS_TCP,DATAGRAM) * modbus_datagram;
    RECORD(MODBUS_STATE,RELATE) * cmd_relate;

    // get datagram from recv_msg
    ret=message_get_record(recv_msg,&modbus_datagram,0);
    if(ret<0)
        return ret;

    // get mbap from message

    MSG_EXPAND * msg_expand;
    ret = message_remove_expand(recv_msg,TYPE_PAIR(MODBUS_TCP,MBAP),&msg_expand);
    if(msg_expand == NULL)
        return -EINVAL;
    modbus_mbap=msg_expand->expand;     
    
    // get server_index

   /* 
   db_record =  memdb_find_first(TYPE_PAIR(MODBUS_STATE,SERVER),"unit_addr",&cmd_relate->unit_addr);
   if(db_record == NULL)
        return -EINVAL;
   client_index = db_record->record;
   client_index->state_machine = MODBUS_RESPONSE;
   */

    // build cmd_relate 
    cmd_relate = Talloc0(sizeof(*cmd_relate));
    if(cmd_relate==NULL)
        return -ENOMEM;
        
    cmd_relate->unit_addr = modbus_datagram->unit_id;
    cmd_relate->no = modbus_mbap->trans_id;

   void * send_msg;
   send_msg = message_create(TYPE(MODBUS_CMD),modbus_datagram->function,recv_msg);
   if(send_msg == NULL)
	   return -EINVAL;

    void * cmd_template = memdb_get_template(TYPE(MODBUS_CMD),modbus_datagram->function);
    if(cmd_template == NULL)
        return -EINVAL;
  
    void * modbus_cmd = Talloc0(struct_size(cmd_template));
    if(modbus_cmd == NULL)
        return -EINVAL;

    ret = blob_2_struct(modbus_datagram->data,modbus_cmd,cmd_template);
    if(ret<0)
        return ret;
       
    ret = message_add_record(send_msg,modbus_cmd);
    if(ret<0)
	    return ret;

    ret = message_add_expand_data(send_msg,TYPE_PAIR(MODBUS_STATE,RELATE),cmd_relate);
    if(ret<0)
	    return ret;

    ret=ex_module_sendmsg(sub_proc,send_msg);
    return ret;
}

RECORD(MODBUS_TCP, DATAGRAM) *  proc_data_datagram(void * recv_msg,int unit_addr)
{
    int ret;
    RECORD(MODBUS_TCP,DATAGRAM) * datagram;
    int  type,subtype;
    UINT16 function_code;
    void * data_template;
    BYTE data_buf[DIGEST_SIZE*8];

    datagram = Talloc0(sizeof(*datagram));
    if(datagram==NULL)
	    return -ENOMEM;

    type = message_get_type(recv_msg);
    function_code = message_get_subtype(recv_msg);

    data_template = memdb_get_template(type,function_code);
    if(data_template == NULL)
        return -EINVAL;
    void * modbus_data;
    ret=message_get_record(recv_msg,&modbus_data,0);
    if(ret<0)
        return ret;
    datagram->datasize=struct_2_blob(modbus_data,data_buf,data_template);
    if(datagram->datasize<0)
	 return datagram->datasize;
    datagram->datasize;
    datagram->data = Talloc0(datagram->datasize);
    if(datagram->data == NULL)
	    return -ENOMEM;
    Memcpy(datagram->data,data_buf,datagram->datasize);
    datagram->unit_id=unit_addr;
    datagram->function = function_code;
    return datagram;
}

int proc_slave_data(void * sub_proc,void * recv_msg)
{
    int ret = 0;
    RECORD(MODBUS_STATE,CLIENT) * client_index;
    RECORD(MODBUS_STATE,RELATE) * cmd_relate;
    RECORD(MODBUS_TCP,MBAP) * modbus_mbap;
    RECORD(MODBUS_TCP,DATAGRAM) * modbus_datagram;

    MSG_EXPAND * msg_expand;
    ret = message_remove_expand(recv_msg,TYPE_PAIR(MODBUS_STATE,RELATE),&msg_expand);
    if(msg_expand == NULL)
        return -EINVAL;
    cmd_relate = msg_expand->expand;     
    

    // build mbap head 
    modbus_mbap = Talloc0(sizeof(*modbus_mbap));
    if(modbus_mbap == NULL)
	    return -ENOMEM;
    modbus_mbap->trans_id=cmd_relate->no;
    modbus_mbap->protocol_id=0;
    modbus_mbap->length_hi=0;
    modbus_mbap->length_lo=0;
   
    // build modbus datagram
    modbus_datagram = proc_data_datagram(recv_msg,cmd_relate->unit_addr);
    if(modbus_datagram == NULL)
	    return -EINVAL;

   void * send_msg;
   send_msg = message_create(TYPE_PAIR(MODBUS_TCP,DATAGRAM),recv_msg);
   if(send_msg == NULL)
	   return -EINVAL;
    ret = message_add_record(send_msg,modbus_datagram);
    if(ret<0)
	    return ret;

    ret = message_add_expand_data(send_msg,TYPE_PAIR(MODBUS_TCP,MBAP),modbus_mbap);
    if(ret<0)
	    return ret;
    ret=ex_module_sendmsg(sub_proc,send_msg);
    return ret;
}
