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
#include "zuc.h"

#include "modbus_tcp.h"
#include "modbus_state.h"
#include "modbus_cmd.h"

#include "modbus_zuc_crypt.h"

#define MAX_LINE_LEN 1024

//BYTE Buf[DIGEST_SIZE*64];
BYTE * key;
BYTE * iv;

ZUC_STATE * zuc_state;
int last_transid;

BYTE * zuc_buf;
BYTE * key_buf;

int mode = 0;

int modbus_zuc_crypt_init(void * sub_proc,void * para)
{
    
    int ret;
    struct zuc_init_struct * init_para=(struct zuc_init_struct *)para;

    mode = init_para->mode;

    zuc_state = Dalloc0(sizeof(*zuc_state),sub_proc);
    if(zuc_state == NULL)
	    return -ENOMEM;

    key = Dalloc0(16,0);   
    iv = Dalloc0(16,0);
    //last_transid = Dalloc0(sizeof(int),0);
        

    key = Dalloc0(16,0);   
    zuc_buf = Dalloc0(256,0);
    key_buf = Dalloc0(256,0);

    last_transid=0;
    if(mode == 0)
    {
	// server(slave) 端密钥初始化，注意密钥在(GENERAL_RETURN,UUID)数据库中
    	RECORD(GENERAL_RETURN,UUID) * key_list;
	key_list = memdb_get_first_record(TYPE_PAIR(GENERAL_RETURN,UUID));
	if(key_list == NULL)
		return -EINVAL;
	Memcpy(key,key_list->return_value,DIGEST_SIZE/2);
	Memcpy(iv,key_list->return_value+DIGEST_SIZE/2,DIGEST_SIZE/2);
	// server(slave) 端密钥初始化结束
    	zuc_init(zuc_state, key, iv);

    }
    else if(mode ==1)
    {
	// client(master) 端密钥初始化，注意密钥在(MODBUS_STATE,SLAVE)数据库中
    	RECORD(MODBUS_STATE,SLAVE) * slave_index;
	slave_index = memdb_get_first_record(TYPE_PAIR(MODBUS_STATE,SLAVE));
	if(slave_index == NULL)
		return -EINVAL;
	Memcpy(key,slave_index->slave_key,DIGEST_SIZE/2);
	Memcpy(iv,slave_index->slave_key+DIGEST_SIZE/2,DIGEST_SIZE/2);
	// client(master) 端密钥初始化结束
    	zuc_init(zuc_state, key, iv);
    }

    return 0;
}

int modbus_zuc_crypt_start(void * sub_proc,void * para)
{
    int ret = 0;
    int rc = 0;
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
        else if((type==TYPE(MODBUS_TCP))&& (subtype  == SUBTYPE(MODBUS_TCP,DATAGRAM)))
        {
	    char * sender = message_get_sender(recv_msg);
	    printf("enter zuc crypt func sender %s\n",sender);
    	    if(mode == 0)  // server(slave) side
	    {
		if(Strcmp(sender,"modbus_slave")==0)
		{
			proc_modbus_zuc_crypt(sub_proc,recv_msg);
		}
	    	else
	    		ex_module_sendmsg(sub_proc,recv_msg);

	    }  	
	    else
	    {		    
		if(Strcmp(sender,"modbus_channel")==0)
		{
			proc_modbus_zuc_decrypt(sub_proc,recv_msg);
		}
	    	else
	    		ex_module_sendmsg(sub_proc,recv_msg);

            }
        }
    }
    return 0;
}

int proc_modbus_zuc_crypt(void * sub_proc,void * recv_msg)
{
    int ret = 0;
    RECORD(MODBUS_TCP,MBAP) * modbus_mbap;
    RECORD(MODBUS_TCP,DATAGRAM) * modbus_datagram;
    int segment_size=256;

    // get datagram from recv_msg
    ret=message_get_record(recv_msg,&modbus_datagram,0);
    if(ret<0)
        return ret;

    printf("enter proc_modbus_zuc func\n ");
    // get mbap from message

    MSG_EXPAND * msg_expand;
    ret = message_remove_expand(recv_msg,TYPE_PAIR(MODBUS_TCP,MBAP),&msg_expand);
    if(msg_expand == NULL)
        return -EINVAL;
    modbus_mbap=msg_expand->expand;     
    
    // 祖冲之加密算法开始


    //祖冲之加密算法结束

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

int proc_modbus_zuc_decrypt(void * sub_proc,void * recv_msg)
{
    int ret = 0;
    RECORD(MODBUS_TCP,MBAP) * modbus_mbap;
    RECORD(MODBUS_TCP,DATAGRAM) * modbus_datagram;
    BYTE zuc_buf[256];
    BYTE key_buf[256];
    int segment_size=256;

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
    
    printf("enter proc_modbus_zuc func 2 last_transid %d trans_id %d\n ",last_transid,modbus_mbap->trans_id);
    // 祖冲之解密算法开始


    //祖冲之解密算法结束

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
