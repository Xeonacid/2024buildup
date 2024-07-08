#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/time.h>
 
#include "data_type.h"
#include "cube.h"
#include "cube_define.h"
#include "cube_record.h"
#include "user_define.h"
#include "plc_emu.h"
#include "score_compute.h"
// add para lib_include
//
int center_probe_init(void * sub_proc, void * para)
{
	int ret;
	// add yorself's module init func here
	return 0;
}

int center_probe_start(void * sub_proc, void * para)
{
	int ret;
	void * recv_msg;
	int type;
	int subtype;
	// add yorself's module exec func here
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
		else if((type==TYPE(USER_DEFINE))&&(subtype==SUBTYPE(USER_DEFINE,RETURN)))
		{
			ret=proc_login_probe(sub_proc,recv_msg);
		}
		else if((type==TYPE(PLC_ENGINEER))&&(subtype==SUBTYPE(PLC_ENGINEER,LOGIC_RETURN)))
		{
			ret=proc_code_upload_probe(sub_proc,recv_msg);
		}
	}
	return 0;
}

int proc_login_probe(void * sub_proc,void * recv_msg)
{
	int ret;

	return ex_module_sendmsg(sub_proc,recv_msg);
}
int proc_code_upload_probe(void * sub_proc,void * recv_msg)
{
	int ret;

	return ex_module_sendmsg(sub_proc,recv_msg);
}
