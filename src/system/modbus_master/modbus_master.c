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
#include "cube.h"
#include "cube_define.h"
#include "cube_record.h"
/*
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
*/
#include "modbus_master.h"
#include "modbus_tcp.h"
#include "modbus_state.h"
#include "modbus_cmd.h"

#define MAX_LINE_LEN 1024

static BYTE Buf[DIGEST_SIZE*64];
struct tcloud_connector * client_conn;

void * proc_masterindex_init(void * sub_proc,void * recv_msg);


int modbus_client_list_init(void * sub_proc,void * data)
{
    RECORD(MODBUS_STATE,MASTER) * master_index;
    RECORD(MODBUS_STATE,SLAVE) * slave_index;
    RECORD(MODBUS_STATE,CLIENT) * client_index;
    int i;

    slave_index = memdb_get_first_record(TYPE_PAIR(MODBUS_STATE,SLAVE));
    while(slave_index!=NULL)
    {
        client_index = Talloc0(sizeof(*client_index));
        if(client_index == NULL)
            return -ENOMEM;
        Strncpy(client_index->client_name,slave_index->slave_name,DIGEST_SIZE);
        client_index->unit_addr = slave_index->unit_addr;
        client_index->no=0;
        Memcpy(client_index->client_key,slave_index->slave_key,DIGEST_SIZE);
        memdb_store(client_index,TYPE_PAIR(MODBUS_STATE,CLIENT),client_index->client_name);
        slave_index = memdb_get_next_record(TYPE_PAIR(MODBUS_STATE,SLAVE));

    }
    return 0;

}

int modbus_master_init(void * sub_proc,void * para)
{
    
    int ret;
    struct init_para * init_para=(struct init_para *)para;
    RECORD(MODBUS_STATE,MASTER) * master_index;

    master_index = Dalloc0(sizeof(*master_index),sub_proc);
    if(master_index==NULL)
        return -ENOMEM;
    Strncpy(master_index->master_name,init_para->master_name,DIGEST_SIZE);
    master_index->unit_num=init_para->unit_num;
    ex_module_setpointer(sub_proc,master_index);

    modbus_client_list_init(sub_proc,para);
    return 0;
}

int modbus_master_start(void * sub_proc,void * para)
{
    int ret = 0;
    int rc = 0;
    RECORD(MODBUS_STATE,MASTER) * master_index;
    void * recv_msg;
    void * send_msg;
    int type;
    int subtype;


    // create slot_port for modbus cmd

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
        if((type==TYPE(MODBUS_STATE))&&(subtype==SUBTYPE(MODBUS_STATE,SLAVE)))
        {
                ret = proc_masterstate_set(sub_proc,recv_msg);
                if(ret<0)
                    return ret;
            
        }
        else if((type==TYPE(MODBUS_CMD)))
        {
           proc_master_cmd(sub_proc,recv_msg);
        }
        else if((type==TYPE(MODBUS_DATA)))
        {
            master_index =  ex_module_getpointer(sub_proc);
            ret=ex_module_sendmsg(sub_proc,recv_msg);
        }
        else if((type==TYPE(MODBUS_TCP))&& (subtype  == SUBTYPE(MODBUS_TCP,DATAGRAM)))
        {
            proc_master_data(sub_proc,recv_msg);
        }
    }
    return 0;
}

int proc_masterstate_set(void * sub_proc,void * recv_msg)
{
    int ret = 0;
    RECORD(MODBUS_STATE,SLAVE) * slave_index;
    RECORD(MODBUS_STATE,MASTER) * master_index;

    master_index = (RECORD(MODBUS_STATE,MASTER)*) ex_module_getpointer(sub_proc);
    if(master_index == NULL)
        return -EINVAL;

    ret = message_get_record(recv_msg,&slave_index,0);
    if(ret<0)
        return ret;
    if(slave_index ==NULL)
        return -EINVAL;
    slave_index->state_machine=MODBUS_CONNECT;
    
    DB_RECORD * db_record; 
    db_record = memdb_store(slave_index,TYPE_PAIR(MODBUS_STATE,SLAVE),slave_index->slave_name);
    if(db_record == NULL)
        return -EINVAL;

    master_index->slave_uuid= Dalloc0(DIGEST_SIZE*master_index->unit_num,sub_proc);
    if(master_index->slave_uuid==NULL)
        return -ENOMEM;
    Memcpy(master_index->slave_uuid,db_record->head.uuid,DIGEST_SIZE);    

    ret = ex_module_setpointer(sub_proc,master_index);
    return ret;
}

RECORD(MODBUS_TCP, DATAGRAM) *  proc_cmd_datagram(void * recv_msg,int unit_addr)
{
    int ret;
    RECORD(MODBUS_TCP,DATAGRAM) * datagram;
    int  type,subtype;
    UINT16 function_code;
    void * cmd_template;
    BYTE data_buf[DIGEST_SIZE*8];

    datagram = Talloc0(sizeof(*datagram));
    if(datagram==NULL)
	    return -ENOMEM;

    type = message_get_type(recv_msg);
    function_code = message_get_subtype(recv_msg);

    cmd_template = memdb_get_template(type,function_code);
    if(cmd_template == NULL)
        return -EINVAL;
    void * modbus_cmd;
    ret=message_get_record(recv_msg,&modbus_cmd,0);
    if(ret<0)
        return ret;
    datagram->datasize=struct_2_blob(modbus_cmd,data_buf,cmd_template);
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


int proc_master_cmd(void * sub_proc,void * recv_msg)
{
    int ret = 0;
    RECORD(MODBUS_STATE,CLIENT) * client_index;
    RECORD(GENERAL_RETURN,UUID) * uuid_expand;
    RECORD(MODBUS_STATE,RELATE) * cmd_relate;
    RECORD(MODBUS_TCP,MBAP) * modbus_mbap;
    RECORD(MODBUS_TCP,DATAGRAM) * modbus_datagram;

    MSG_EXPAND * msg_expand;
    ret = message_remove_expand(recv_msg,TYPE_PAIR(GENERAL_RETURN,UUID),&msg_expand);
    if(msg_expand == NULL)
        return -EINVAL;
    uuid_expand=msg_expand->expand;     
    
    DB_RECORD * db_record;
   
    char comp_name[32];

    Memset(comp_name,0,32);
    Strncpy(comp_name,uuid_expand->name,32);


    db_record = memdb_find_first(TYPE_PAIR(MODBUS_STATE,CLIENT),"client_name",comp_name);
    if(db_record == NULL)
        return -EINVAL;
    client_index = db_record->record;

    client_index->no++;    

    cmd_relate = Talloc0(sizeof(*cmd_relate));
    if(cmd_relate==NULL)
        return -ENOMEM;

    cmd_relate->unit_addr=client_index->unit_addr;
    cmd_relate->no=client_index->no;
    Memcpy(cmd_relate->msg_uuid,uuid_expand->return_value,DIGEST_SIZE);

    db_record = memdb_store(cmd_relate,TYPE_PAIR(MODBUS_STATE,RELATE),client_index->client_name);
    if(db_record == NULL)
        return -EINVAL;

    db_record = memdb_store(client_index,TYPE_PAIR(MODBUS_STATE,CLIENT),client_index->client_name);
    if(db_record == NULL)
        return -EINVAL;

    // build mbap head 
    modbus_mbap = Talloc0(sizeof(*modbus_mbap));
    if(modbus_mbap == NULL)
	    return -ENOMEM;
    modbus_mbap->trans_id=client_index->no;
    modbus_mbap->protocol_id=0;
    modbus_mbap->length_hi=0;
    modbus_mbap->length_lo=0;
   
    // build modbus datagram
    modbus_datagram = proc_cmd_datagram(recv_msg,client_index->unit_addr);
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

int proc_master_data(void * sub_proc,void * recv_msg)
{
    int ret = 0;
    RECORD(MODBUS_STATE,CLIENT) * client_index;
    RECORD(GENERAL_RETURN,UUID) * uuid_expand;
    RECORD(MODBUS_STATE,RELATE) * cmd_relate;
    RECORD(MODBUS_TCP,MBAP) * modbus_mbap;
    RECORD(MODBUS_TCP,DATAGRAM) * modbus_datagram;

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
    

    BYTE uuid[DIGEST_SIZE];

    // find cmd_relate from memdb
    cmd_relate = Talloc0(sizeof(*cmd_relate));
    if(cmd_relate==NULL)
        return -ENOMEM;

     
    cmd_relate->unit_addr = modbus_datagram->unit_id;
    cmd_relate->no = modbus_mbap->trans_id;

    memdb_comp_record_uuid(cmd_relate,TYPE_PAIR(MODBUS_STATE,RELATE),uuid);

    DB_RECORD * db_record;
   
    db_record = memdb_remove(uuid,TYPE_PAIR(MODBUS_STATE,RELATE));
    if(db_record == NULL)
        return -EINVAL;

    Free0(cmd_relate);    
    cmd_relate = db_record->record;


    // get client_index
    
   db_record =  memdb_find_first(TYPE_PAIR(MODBUS_STATE,CLIENT),"unit_addr",&cmd_relate->unit_addr);
   if(db_record == NULL)
        return -EINVAL;
   client_index = db_record->record;
   client_index->state_machine = MODBUS_RESPONSE;

   // get uuid_expand
   uuid_expand = Talloc0(sizeof(*uuid_expand));
   if(uuid_expand == NULL)
        return -ENOMEM;

    uuid_expand->name = dup_str(client_index->client_name,0);
    Memcpy(uuid_expand->return_value,cmd_relate->msg_uuid,DIGEST_SIZE);

    db_record = memdb_store(client_index,TYPE_PAIR(MODBUS_STATE,CLIENT),client_index->client_name);
    if(db_record == NULL)
        return -EINVAL;

   void * send_msg;
   send_msg = message_create(TYPE(MODBUS_DATA),modbus_datagram->function,recv_msg);
   if(send_msg == NULL)
	   return -EINVAL;

    void * data_template = memdb_get_template(TYPE(MODBUS_DATA),modbus_datagram->function);
    if(data_template == NULL)
        return -EINVAL;
  
    void * modbus_data = Talloc0(struct_size(data_template));
    if(modbus_data == NULL)
        return -EINVAL;

    ret = blob_2_struct(modbus_datagram->data,modbus_data,data_template);
    if(ret<0)
        return ret;
       
    ret = message_add_record(send_msg,modbus_data);
    if(ret<0)
	    return ret;

    ret = message_add_expand_data(send_msg,TYPE_PAIR(GENERAL_RETURN,UUID),uuid_expand);
    if(ret<0)
	    return ret;
    ret=ex_module_sendmsg(sub_proc,send_msg);
    return ret;
}
