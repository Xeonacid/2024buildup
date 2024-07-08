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
int event_judge_init(void * sub_proc, void * para)
{
	int ret;
	// add yorself's module init func here
	return 0;
}

int event_judge_start(void * sub_proc, void * para)
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
			ret=proc_engineer_login(sub_proc,recv_msg);
		}
		else if((type==TYPE(PLC_ENGINEER))&&(subtype==SUBTYPE(PLC_ENGINEER,LOGIC_RETURN)))
		{
			ret=proc_code_upload_judge(sub_proc,recv_msg);
		}
		else if((type==TYPE(PLC_OPERATOR))&&(subtype==SUBTYPE(PLC_OPERATOR,PLC_RETURN)))
		{
			ret=proc_operator_cmd_judge(sub_proc,recv_msg);
		}
	}
	return 0;
}

int proc_engineer_login(void * sub_proc,void * recv_msg)
{
	int ret;
	RECORD(USER_DEFINE,RETURN) * user_login;
	RECORD(USER_DEFINE, SERVER_STATE) * user_info;
	RECORD(SCORE_COMPUTE,EVENT) * score_event;

	MSG_EXPAND * msg_expand;
	DB_RECORD * db_record;
	void * new_msg;
	int i;
	int elem_no;
	void * record_template;


	//获取已完成访控处理的数据 
	ret=message_get_record(recv_msg,&user_login,0);
	if(ret<0)
		return ret;

   

	// 创建事件，该事件为背景测试的用户登录事件，如登录成功，则事件成功，
	// 否则事件失败
	
	score_event=Talloc0(sizeof(*score_event));
	if(score_event == NULL)
		return -ENOMEM;

	score_event->item_name = dup_str("background_test",0);
	score_event->name = dup_str("engineer_login",0);
	
	if(user_login->return_code == SUCCEED)
	       	score_event->result=SCORE_RESULT_SUCCEED;	
	else
	       	score_event->result=SCORE_RESULT_FAIL;	

	new_msg=message_create(TYPE_PAIR(SCORE_COMPUTE,EVENT),NULL);
	if(new_msg==NULL)
		return -EINVAL;
	message_add_record(new_msg,score_event);
	ret=ex_module_sendmsg(sub_proc,new_msg);
	return ret;
}
int proc_code_upload_judge(void * sub_proc,void * recv_msg)
{
	int ret;
	RECORD(PLC_ENGINEER,LOGIC_RETURN) * code_upload;
	RECORD(USER_DEFINE, SERVER_STATE) * user_info;
	RECORD(SCORE_COMPUTE,EVENT) * score_event;

	MSG_EXPAND * msg_expand;
	DB_RECORD * db_record;
	void * new_msg;
	int i;
	int elem_no;
	void * record_template;

	//获取已完成访控处理的数据 
	ret=message_get_record(recv_msg,&code_upload,0);
	if(ret<0)
		return ret;

	// 根据用户名获取主体标记
	db_record=memdb_find_first(TYPE_PAIR(USER_DEFINE,SERVER_STATE),"user_name",code_upload->author);

	if(db_record==NULL)
	{
		print_cubeerr("event_judge: can't find author!\n");
		return -EINVAL;
	}
	
	user_info=db_record->record;

	// 创建事件，该事件为背景测试的代码上传事件，如为工程师上传，则事件成功，
	// 否则事件失败
	
	score_event=Talloc0(sizeof(*score_event));
	if(score_event == NULL)
		return -ENOMEM;

	score_event->item_name = dup_str("background_test",0);
	if(Strcmp(code_upload->logic_filename,"thermostat_logic.c") == 0)
		score_event->name = dup_str("code_upload",0);
	else if(Strcmp(code_upload->logic_filename,"libthermostat_logic.so") == 0)
		score_event->name = dup_str("bin_upload",0);
	
	if(user_info->role == PLC_ENGINEER)
	       	score_event->result=SCORE_RESULT_SUCCEED;	
	else
	       	score_event->result=SCORE_RESULT_FAIL;	

	new_msg=message_create(TYPE_PAIR(SCORE_COMPUTE,EVENT),NULL);
	if(new_msg==NULL)
		return -EINVAL;
	message_add_record(new_msg,score_event);
	ret=ex_module_sendmsg(sub_proc,new_msg);
	return ret;
}
int proc_operator_cmd_judge(void * sub_proc,void * recv_msg)
{
	int ret;
	RECORD(PLC_OPERATOR,PLC_RETURN) * plc_return;
	RECORD(USER_DEFINE, SERVER_STATE) * user_info;
	RECORD(SCORE_COMPUTE,EVENT) * score_event;

	MSG_EXPAND * msg_expand;
	DB_RECORD * db_record;
	void * new_msg;
	int i;
	int elem_no;
	void * record_template;

	//获取PLC返回命令 
	ret=message_get_record(recv_msg,&plc_return,0);
	if(ret<0)
		return ret;

	// 创建事件，该事件为背景测试的命令执行事件，共三个事件：启动，观察温度和设置温度
	
	score_event=Talloc0(sizeof(*score_event));
	if(score_event == NULL)
		return -ENOMEM;

	score_event->item_name = dup_str("background_test",0);

	if(plc_return->action==ACTION_ON)
	{
		score_event->name = dup_str("plc_heating_on",0);
		// 开启事件
		if(Strcmp(plc_return->action_desc,"heating_S")==0)
	       		score_event->result=SCORE_RESULT_SUCCEED;	
		else
	       		score_event->result=SCORE_RESULT_FAIL;	
	}
	else if(plc_return->action==ACTION_MONITOR)
	{
		// 监控事件
		score_event->name = dup_str("plc_watch",0);
		// 开启事件
		if(Strcmp(plc_return->action_desc,"curr_T")==0)
	       		score_event->result=SCORE_RESULT_SUCCEED;	
		else
	       		score_event->result=SCORE_RESULT_FAIL;	
	}
	else if(plc_return->action==ACTION_ADJUST)
	{
		// 监控事件
		score_event->name = dup_str("plc_set",0);
		// 开启事件
	       	score_event->result=SCORE_RESULT_FAIL;	
		if(Strcmp(plc_return->action_desc,"set_T")==0)
			if(plc_return->value == 3900)
	       			score_event->result=SCORE_RESULT_SUCCEED;	
	}

	new_msg=message_create(TYPE_PAIR(SCORE_COMPUTE,EVENT),NULL);
	if(new_msg==NULL)
		return -EINVAL;
	message_add_record(new_msg,score_event);
	ret=ex_module_sendmsg(sub_proc,new_msg);
	return ret;
}
