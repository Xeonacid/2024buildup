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

char * operator_sender = "modbus_slave";
char * plc_sender = "device";
int mode =0;   // 0 means normal, 1 means prohibit plc

int operator_shield_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
	mode =0;
	return 0;
}

int operator_shield_start(void * sub_proc,void * para)
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
		if(type==TYPE(MODBUS_CMD))
		{
			ret= proc_operator_shield(sub_proc,recv_msg);
		}
	}
	return 0;
}

int proc_operator_shield(void * sub_proc,void * recv_msg)
{
	int i;
	int ret;
	int subtype;

	int illegal_cmd =0;

	// 在这里添加命令验证逻辑
	
	char * sender =message_get_sender(recv_msg);

	if(Strcmp(sender,operator_sender)==0)
	{
		subtype=message_get_subtype(recv_msg);
		if(subtype ==  SUBTYPE(MODBUS_CMD,WRITE_SINGLE_REGISTER))
		{
			illegal_cmd=0;
			mode=1;
		}
	}
	else if(Strcmp(sender,plc_sender)==0)
	{
		if(mode==1)
			illegal_cmd=1;
		else if (mode ==0)
			illegal_cmd=0;

	}

	// 命令验证逻辑结束


	if(illegal_cmd)
	{
		return -1;
	}
	else
		ex_module_sendmsg(sub_proc,recv_msg);

	return ret;
}
