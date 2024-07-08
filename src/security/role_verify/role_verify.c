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

extern struct timeval time_val={0,50*1000};

int role_verify_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
	return 0;
}

int role_verify_start(void * sub_proc,void * para)
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
		if((type==TYPE(USER_DEFINE))&&(subtype==SUBTYPE(USER_DEFINE,LOGIN)))
		{
			ret= proc_role_verify(sub_proc,recv_msg);
		}
	}
	return 0;
}

int proc_role_verify(void * sub_proc,void * recv_msg)
{
	int i;
	int ret;

	RECORD(USER_DEFINE,LOGIN) * user_login;
	RECORD(USER_DEFINE,RETURN) * user_return;
	RECORD(USER_DEFINE,SERVER_STATE) * user_state;
	DB_RECORD * db_record;
	int illegal_user =0;

	ret=message_get_record(recv_msg,&user_login,0);
	if(ret<0)
		return ret;
	// 在这里添加角色验证逻辑
	
	db_record=memdb_find_first(TYPE_PAIR(USER_DEFINE,SERVER_STATE),"user_name",user_login->user_name);
	if(db_record !=NULL)
	{
		user_state=db_record->record;

		if(user_state->role == PLC_ENGINEER)
		{
			if(Strcmp(user_login->proc_name,"engineer_station")!=0)
				illegal_user=1;
		}
		else if(user_state->role == PLC_OPERATOR)
		{
			if(Strcmp(user_login->proc_name,"operator_station")!=0)
				illegal_user=1;
		}
		else if(user_state->role == PLC_MONITOR)
		{
			if(Strcmp(user_login->proc_name,"monitor_term")!=0)
				illegal_user=1;
		}
	}

	// 添加角色验证逻辑结束


	if(illegal_user)
	{
		user_return = Talloc0(sizeof(*user_return));
	       if(user_return == NULL)
	       		return -ENOMEM;
		user_return->return_code = INVALID;
		user_return->return_info=dup_str("not right user!\n",0);
		void * send_msg = message_create(TYPE_PAIR(USER_DEFINE,RETURN),recv_msg);
		message_add_record(send_msg,user_return);	
		ex_module_sendmsg(sub_proc,send_msg);
	}

	else
		ex_module_sendmsg(sub_proc,recv_msg);

	
	//角色验证代码结束

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
