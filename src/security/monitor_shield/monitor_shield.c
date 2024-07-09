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
#include "modbus_cmd.h"

extern struct timeval time_val={0,50*1000};

char * operator_route = "operator_cmd";
char * monitor_route = "monitor_ctrl";
int mode =0;   // 0 means normal, 1 means prohibit plc

int monitor_shield_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
	mode =0;
	return 0;
}

int monitor_shield_start(void * sub_proc,void * para)
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
		if((type==TYPE(PLC_OPERATOR)) && (subtype== SUBTYPE(PLC_OPERATOR,PLC_CMD)))
		{
			ret= proc_monitor_shield(sub_proc,recv_msg);
		}
	}
	return 0;
}

int proc_monitor_shield(void * sub_proc,void * recv_msg)
{
	int i;
	int ret;
	int subtype;

	int illegal_cmd =0;
	RECORD(PLC_OPERATOR,PLC_CMD) * plc_cmd;

	ret = message_get_record(recv_msg,&plc_cmd,0);
	if(ret<0)
		return ret;


	// 在这里添加命令验证逻辑
	
	MSG_HEAD * msg_head;
	
	msg_head= message_get_head(recv_msg);

	if(Strcmp(monitor_route,msg_head->route)==0)
	{

		if(plc_cmd->action == ACTION_ADJUST)
		{
			illegal_cmd=0;
			mode=1;
		}
	}
	else if(Strcmp(operator_route,msg_head->route)==0)
	{
		if(mode==1)
			illegal_cmd=1;
		else if (mode ==0)
			illegal_cmd=0;

	}

	// 命令验证逻辑结束


	if(illegal_cmd)
	{
   		RECORD(PLC_OPERATOR,PLC_RETURN) * plc_return;
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
