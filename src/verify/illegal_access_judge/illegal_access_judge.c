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
int illegal_access_judge_init(void * sub_proc, void * para)
{
	int ret;
	// add yorself's module init func here
	return 0;
}

int illegal_access_judge_start(void * sub_proc, void * para)
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
		else if((type==TYPE(PLC_ENGINEER))&&(subtype==SUBTYPE(PLC_ENGINEER,LOGIC_UPLOAD)))
		{
			ret=proc_illegal_code_upload_judge(sub_proc,recv_msg);
		}
		else if((type==TYPE(PLC_OPERATOR))&&(subtype==SUBTYPE(PLC_OPERATOR,PLC_CMD)))
		{
			ret=proc_illegal_operator_cmd_judge(sub_proc,recv_msg);
		}
	}
	return 0;
}

int proc_illegal_code_upload_judge(void * sub_proc,void * recv_msg)
{
	int ret;
	RECORD(PLC_ENGINEER,LOGIC_UPLOAD) * code_upload;
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
		print_cubeerr("illegal_access_judge: can't find author!\n");
		return -EINVAL;
	}
	
	user_info=db_record->record;

	// 创建事件，该事件为背景测试的代码上传事件，如为工程师上传，则事件成功，
	// 否则事件失败
	
	score_event=Talloc0(sizeof(*score_event));
	if(score_event == NULL)
		return -ENOMEM;

	score_event->item_name = dup_str("illegal_access",0);
	if(Strcmp(code_upload->logic_filename,"thermostat_logic.c") == 0)
	{
		if(user_info->role != PLC_ENGINEER)
		{
			if(user_info->role == PLC_MONITOR)
				score_event->name = dup_str("monitor_upload",0);
			else if(user_info->role == PLC_OPERATOR)
				score_event->name = dup_str("operator_upload",0);
	
	       		score_event->result=SCORE_RESULT_SUCCEED;	
			new_msg=message_create(TYPE_PAIR(SCORE_COMPUTE,EVENT),NULL);
			if(new_msg==NULL)
				return -EINVAL;
			message_add_record(new_msg,score_event);
			ret=ex_module_sendmsg(sub_proc,new_msg);
		}
	}
	return ret;
}
int proc_illegal_operator_cmd_judge(void * sub_proc,void * recv_msg)
{
	int ret;
	RECORD(PLC_OPERATOR,PLC_CMD) * plc_cmd;
	RECORD(USER_DEFINE, SERVER_STATE) * user_info;
	RECORD(SCORE_COMPUTE,EVENT) * score_event;
	RECORD(GENERAL_RETURN,STRING) * site_info;

	MSG_EXPAND * msg_expand;
	DB_RECORD * db_record;
	void * new_msg;
	int i;
	int elem_no;
	void * record_template;

	//获取PLC返回命令 
	ret=message_get_record(recv_msg,&plc_cmd,0);
	if(ret<0)
		return ret;


	// 获取扩展项信息
	ret = message_get_define_expand(recv_msg,&msg_expand,TYPE_PAIR(GENERAL_RETURN,STRING));
	if(ret<0)
		return ret;
	if(msg_expand ==NULL)
		return -EINVAL;
	site_info = msg_expand->expand;

	// 获取用户信息
	db_record=memdb_find_first(TYPE_PAIR(USER_DEFINE,SERVER_STATE),"user_name",
			plc_cmd->plc_operator);

	if(db_record==NULL)
	{
		print_cubeerr("illegal_access_judge: can't find operator!");
		return -EINVAL;
	}
	
	user_info=db_record->record;

	// 创建事件，该事件为背景测试的命令执行事件，共三个事件：启动，观察温度和设置温度
	
	score_event=Talloc0(sizeof(*score_event));
	if(score_event == NULL)
		return -ENOMEM;

	score_event->item_name = dup_str("illegal_access",0);


	if(plc_cmd->action==ACTION_ADJUST) // 温度调节行为是受限操作
	{
		if(Strcmp(site_info->return_value,"operator_station")==0)
		{
			//操作员站的操作
			if(user_info->role == PLC_ENGINEER)
				score_event->name = dup_str("engineer_adjust_in_OS",0);
			else if(user_info->role == PLC_MONITOR)
				score_event->name = dup_str("monitor_adjust_in_OS",0);
			else 
				score_event->name = NULL;
		}
		else if(Strcmp(site_info->return_value,"center_station")==0)
		{
			//管理中心的操作
			if(user_info->role == PLC_ENGINEER)
				score_event->name = dup_str("engineer_adjust_in_center",0);
			else if(user_info->role == PLC_OPERATOR)
				score_event->name = dup_str("operator_adjust_in_center",0);
			else 
				score_event->name = NULL;
		}
	       	score_event->result=SCORE_RESULT_SUCCEED;	
		
	}

	if(score_event->name != NULL)
	{
		new_msg=message_create(TYPE_PAIR(SCORE_COMPUTE,EVENT),NULL);
		if(new_msg==NULL)
			return -EINVAL;
		message_add_record(new_msg,score_event);
		ret=ex_module_sendmsg(sub_proc,new_msg);
	}
	return ret;
}
