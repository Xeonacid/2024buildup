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
		else if((type==TYPE(GENERAL_RETURN))&&(subtype==SUBTYPE(GENERAL_RETURN,STRING)))
		{
			ret=proc_probe_item_select(sub_proc,recv_msg);
		}
		else if((type==TYPE(USER_DEFINE))&&(subtype==SUBTYPE(USER_DEFINE,RETURN)))
		{
			ret=proc_login_probe(sub_proc,recv_msg);
		}
		else if((type==TYPE(PLC_ENGINEER))&&(subtype==SUBTYPE(PLC_ENGINEER,LOGIC_RETURN)))
		{
			ret=proc_code_upload_probe(sub_proc,recv_msg);
		}
		else if((type==TYPE(PLC_ENGINEER))&&(subtype==SUBTYPE(PLC_ENGINEER,LOGIC_UPLOAD)))
		{
			ret=proc_upload_cmd_probe(sub_proc,recv_msg);
		}
	}
	return 0;
}

int proc_probe_item_select(void * sub_proc,void * recv_msg)
{
	int ret;
	RECORD(GENERAL_RETURN,STRING) * item_select;
	ret=message_get_record(recv_msg,&item_select,0);
	if(ret<0)
		return ret;
	char proc_name[32];

	proc_share_data_getvalue("proc_name",proc_name);

	item_select->return_value = dup_str(proc_name,32);

	ex_module_setpointer(sub_proc,item_select);
	return ex_module_sendmsg(sub_proc,recv_msg);
}
int proc_login_probe(void * sub_proc,void * recv_msg)
{
	int ret;
	void * send_msg;
	RECORD(GENERAL_RETURN,STRING) * item_select;
	RECORD(USER_DEFINE,RETURN) * user_define;

	ret=message_get_record(recv_msg,&user_define,0);
	if(ret<0)
		return ret;

	send_msg = message_create(TYPE_PAIR(USER_DEFINE,RETURN),NULL);
	if(send_msg == NULL)
		return -EINVAL;
	message_add_record(send_msg,user_define);

	item_select = ex_module_getpointer(sub_proc);
	if(item_select !=NULL)
		message_add_expand_data(send_msg,TYPE_PAIR(GENERAL_RETURN,STRING),item_select);

	return ex_module_sendmsg(sub_proc,send_msg);
}
int proc_code_upload_probe(void * sub_proc,void * recv_msg)
{
	int ret;
	void * send_msg;
	RECORD(GENERAL_RETURN,STRING) * item_select;
	RECORD(PLC_ENGINEER,LOGIC_RETURN) * plc_return;

	item_select = ex_module_getpointer(sub_proc);

	if(item_select !=NULL)
	{
		if(Strcmp(item_select->name,"illegal_access")==0)
			return 0;
	}

	ret=message_get_record(recv_msg,&plc_return,0);
	if(ret<0)
		return ret;

	send_msg = message_create(TYPE_PAIR(PLC_ENGINEER,LOGIC_RETURN),NULL);
	if(send_msg == NULL)
		return -EINVAL;
	message_add_record(send_msg,plc_return);

	if(item_select !=NULL)
		message_add_expand_data(send_msg,TYPE_PAIR(GENERAL_RETURN,STRING),item_select);

	return ex_module_sendmsg(sub_proc,send_msg);
}
int proc_upload_cmd_probe(void * sub_proc,void * recv_msg)
{
	int ret;
	void * send_msg;
	RECORD(GENERAL_RETURN,STRING) * item_select;
	RECORD(PLC_ENGINEER,LOGIC_UPLOAD) * plc_cmd;

	item_select = ex_module_getpointer(sub_proc);
	print_cubeaudit("center_probe: dup plc_cmd");

	if(Strcmp(item_select->name,"background_test")==0)
		return 0;

	ret=message_get_record(recv_msg,&plc_cmd,0);
	if(ret<0)
		return ret;

	print_cubeaudit("center_probe: create new message");
	send_msg = message_create(TYPE_PAIR(PLC_ENGINEER,LOGIC_UPLOAD),NULL);
	if(send_msg == NULL)
		return -EINVAL;
	message_add_record(send_msg,plc_cmd);

	if(item_select !=NULL)
		message_add_expand_data(send_msg,TYPE_PAIR(GENERAL_RETURN,STRING),item_select);

	return ex_module_sendmsg(sub_proc,send_msg);
}
