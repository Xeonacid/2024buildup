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
#include <sys/types.h>
#include <pwd.h>

#include "data_type.h"
#include "cube.h"
#include "cube_define.h"
#include "cube_record.h"
#include "file_struct.h"

#include "plc_emu.h"
#include "user_define.h"
#include "logic_store.h"

#define MAX_LINE_LEN 1024

char * code_path = "/root/modbus-demo/src/logic/thermostat_logic/";
char * bin_path = "/root/modbus-demo/src/logic/thermostat_logic/";
char * target_path = "/root/modbus-demo/plugin/";

static unsigned char Buf[DIGEST_SIZE*32];

int logic_store_init(void * sub_proc,void * para)
{
    int ret;
    if(access("logic_code",F_OK)==-1)
        system("mkdir logic_code");
        
    if(access("logic_bin",F_OK)==-1)
        system("mkdir logic_bin");

    return 0;
}

int logic_store_start(void * sub_proc,void * para)
{
    int ret = 0, len = 0, i = 0, j = 0;
    int rc = 0;

    int type,subtype;	
    void * recv_msg;

    // create a slot for 2 message active
    void * slot_port;
    slot_port = slot_port_init("file_recv",2);
    slot_port_addmessagepin(slot_port,TYPE_PAIR(PLC_ENGINEER,LOGIC_UPLOAD));
    slot_port_addmessagepin(slot_port,TYPE_PAIR(FILE_TRANS,FILE_NOTICE));
    ex_module_addslot(sub_proc,slot_port);


    while(1)
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
		if((type==TYPE(PLC_ENGINEER))&&
			(subtype==SUBTYPE(PLC_ENGINEER,LOGIC_UPLOAD)))
		{
			ret=proc_logic_store(sub_proc,recv_msg);
		}
		else if((type==TYPE(FILE_TRANS))&&
			(subtype==SUBTYPE(FILE_TRANS,FILE_NOTICE)))
		{
			ret=proc_logic_store(sub_proc,recv_msg);
		}
	}
    return 0;
}

int proc_logic_store(void * sub_proc,void * recv_msg)
{
    int ret;
    int i=0;
    void * send_msg;
    void * file_msg;
    void * logic_msg;
    time_t curr_time;
    BYTE msg_uuid[DIGEST_SIZE];
    void * slot_port;
    void * sock;

    // get file_recv slot port
   slot_port = ex_module_findport(sub_proc,"file_recv");
   if(slot_port == NULL)
        return -EINVAL;

   // check if there is sock 
   message_get_uuid(recv_msg,msg_uuid);
   sock = ex_module_findsock(sub_proc,msg_uuid);
   if(sock == NULL)
   {
        // no sock, create a new sock
        sock = slot_create_sock(slot_port,msg_uuid);
        ex_module_addsock(sub_proc,sock);
        ret=slot_sock_addmsg(sock,recv_msg);
        return 0;
   }
   else
   {
        ret = slot_sock_addmsg(sock,recv_msg);
        if(ret>0)
        {
            sock=ex_module_removesock(sub_proc,msg_uuid);
            file_msg = slot_sock_removemessage(sock,TYPE_PAIR(FILE_TRANS,FILE_NOTICE));         
            if(file_msg==NULL)
                return -EINVAL;
            logic_msg = slot_sock_removemessage(sock,TYPE_PAIR(PLC_ENGINEER,LOGIC_UPLOAD));         
            if(logic_msg==NULL)
                return -EINVAL;
        }
        else
            return 0;
   }


   RECORD(PLC_ENGINEER,LOGIC_UPLOAD) * logic_upload;
   RECORD(PLC_ENGINEER,LOGIC_CODE) * logic_code;
   RECORD(PLC_ENGINEER,LOGIC_BIN) * logic_bin;
   RECORD(PLC_ENGINEER,LOGIC_RETURN) * logic_return;
   RECORD(FILE_TRANS,REQUEST) * file_req;
   MSG_EXPAND * msg_expand;
   void * type_msg;

    printf("enter logic store func!\n");

    ret = message_get_record(logic_msg,&logic_upload,0);
    if(ret<0)
        return ret;

    if(logic_upload->type  == FILE_CODE)
    {

        ret=message_remove_expand(logic_msg,TYPE_PAIR(PLC_ENGINEER,LOGIC_CODE),&msg_expand);
        if(msg_expand==NULL)
            return -EINVAL;
        logic_code = msg_expand->expand;
        if(logic_code == NULL)
            return -ENOMEM;

        memdb_store(logic_code,TYPE_PAIR(PLC_ENGINEER,LOGIC_CODE),NULL);

        type_msg = message_gen_typesmsg(TYPE_PAIR(PLC_ENGINEER,LOGIC_CODE),NULL);
        ex_module_sendmsg(sub_proc,type_msg);


        logic_return = Talloc0(sizeof(*logic_return));
        if(logic_return == NULL)
            return -ENOMEM;
        logic_return->plc_devname = dup_str(logic_upload->plc_devname,DIGEST_SIZE);
        logic_return->logic_filename = dup_str(logic_upload->logic_filename,DIGEST_SIZE);
        Memcpy(logic_return->uuid,logic_upload->uuid,DIGEST_SIZE);
        logic_return->author = dup_str(logic_upload->author,DIGEST_SIZE);
        logic_return->time=logic_upload->time;


        send_msg = message_create(TYPE_PAIR(PLC_ENGINEER,LOGIC_RETURN),recv_msg);
        if(send_msg==NULL)
            return -EINVAL;
        message_add_record(send_msg,logic_return);

        ex_module_sendmsg(sub_proc,send_msg);

    }
    else if(logic_upload->type == FILE_LOGIC)
    {
    
        ret=message_remove_expand(logic_msg,TYPE_PAIR(PLC_ENGINEER,LOGIC_BIN),&msg_expand);
        if(msg_expand==NULL)
            return -EINVAL;
        logic_bin = msg_expand->expand;
        if(logic_bin == NULL)
            return -ENOMEM;

        memdb_store(logic_bin,TYPE_PAIR(PLC_ENGINEER,LOGIC_BIN),NULL);

        type_msg = message_gen_typesmsg(TYPE_PAIR(PLC_ENGINEER,LOGIC_BIN),NULL);
        ex_module_sendmsg(sub_proc,type_msg);


        logic_return = Talloc0(sizeof(*logic_return));
        if(logic_return == NULL)
            return -ENOMEM;
        logic_return->plc_devname = dup_str(logic_upload->plc_devname,DIGEST_SIZE);
        logic_return->logic_filename = dup_str(logic_upload->logic_filename,DIGEST_SIZE);
        Memcpy(logic_return->uuid,logic_upload->uuid,DIGEST_SIZE);
        logic_return->author = dup_str(logic_upload->author,DIGEST_SIZE);
        logic_return->time=logic_upload->time;


        send_msg = message_create(TYPE_PAIR(PLC_ENGINEER,LOGIC_RETURN),recv_msg);
        if(send_msg==NULL)
            return -EINVAL;
        message_add_record(send_msg,logic_return);

        ex_module_sendmsg(sub_proc,send_msg);
    }
    else
        return -EINVAL;


    return 0; 
}
