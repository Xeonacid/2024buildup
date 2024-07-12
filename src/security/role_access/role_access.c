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

#include "user_define.h"
#include "plc_emu.h"

extern struct timeval time_val={0,50*1000};

int role_access_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
	return 0;
}

int role_access_start(void * sub_proc,void * para)
{
	int ret;
	void * recv_msg;
	int i;
	int type;
	int subtype;


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
			ex_module_sendmsg(sub_proc,recv_msg);
			continue;
		}
		if((type==TYPE(PLC_ENGINEER))&&(subtype==SUBTYPE(PLC_ENGINEER,LOGIC_UPLOAD)))
		{
			ret= proc_engineer_access(sub_proc,recv_msg);
		}
		else if((type==TYPE(PLC_OPERATOR))&&(subtype==SUBTYPE(PLC_OPERATOR,PLC_CMD)))
		{
			ret= proc_operator_access(sub_proc,recv_msg);
		}
	}
	return 0;
}

int proc_engineer_access(void * sub_proc,void * recv_msg)
{
	int i;
	int ret;

	RECORD(PLC_ENGINEER,LOGIC_UPLOAD) * code_upload;
	RECORD(USER_DEFINE,SERVER_STATE) * user_state;
   	RECORD(PLC_ENGINEER,LOGIC_RETURN) * logic_return;
	DB_RECORD * db_record;
	int illegal_user =0;

	ret=message_get_record(recv_msg,&code_upload,0);
	if(ret<0)
		return ret;
	// 在这里添加角色验证逻辑
	

	// 添加角色验证逻辑结束


	if(illegal_user)
	{
        	logic_return = Talloc0(sizeof(*logic_return));
        	if(logic_return == NULL)
            		return -ENOMEM;
        	logic_return->plc_devname = dup_str(code_upload->plc_devname,DIGEST_SIZE);
        	logic_return->logic_filename = dup_str(code_upload->logic_filename,DIGEST_SIZE);
        	Memcpy(logic_return->uuid,code_upload->uuid,DIGEST_SIZE);
        	logic_return->author = dup_str(code_upload->author,DIGEST_SIZE);
        	logic_return->time=code_upload->time;

		logic_return->result=-1;
		void * send_msg = message_create(TYPE_PAIR(PLC_ENGINEER,LOGIC_RETURN),recv_msg);
		message_add_record(send_msg,logic_return);	
		ex_module_sendmsg(sub_proc,send_msg);
	}

	else
		ex_module_sendmsg(sub_proc,recv_msg);


	return ret;
}

int proc_operator_access(void * sub_proc,void * recv_msg)
{
	int i;
	int ret;

	RECORD(PLC_OPERATOR,PLC_CMD) * plc_cmd;
	RECORD(USER_DEFINE,SERVER_STATE) * user_state;
   	RECORD(PLC_OPERATOR,PLC_RETURN) * plc_return;
	DB_RECORD * db_record;
	int illegal_user =0;

	ret=message_get_record(recv_msg,&plc_cmd,0);
	if(ret<0)
		return ret;
	// 在这里添加角色验证逻辑
	

	// 添加角色验证逻辑结束


	if(illegal_user)
	{
        	plc_return = Talloc0(sizeof(*plc_return));
        	if(plc_return == NULL)
            		return -ENOMEM;
        	plc_return->plc_devname = dup_str(plc_cmd->plc_devname,DIGEST_SIZE);
        	plc_return->action = plc_cmd->action;
        	plc_return->action_desc = dup_str(plc_cmd->action_desc,DIGEST_SIZE);
        	plc_return->value = 0;
        	plc_return->time=plc_cmd->time;

		plc_return->result=-1;
		void * send_msg = message_create(TYPE_PAIR(PLC_OPERATOR,PLC_RETURN),recv_msg);
		message_add_record(send_msg,plc_return);	
		ex_module_sendmsg(sub_proc,send_msg);
	}

	else
		ex_module_sendmsg(sub_proc,recv_msg);


	return ret;
}

int proc_ctrl_message(void * sub_proc,void * message)
{
	int ret;
	int i=0;
	printf("begin proc echo \n");

	struct ctrl_message * ctrl_msg;

	
	ret=message_get_record(message,&ctrl_msg,i++);
	if(ret<0)
		return ret;
	if(ctrl_msg!=NULL)
	{
		ret=ctrl_msg->ctrl; 
	}

	ex_module_sendmsg(sub_proc,message);

	return ret;
}
