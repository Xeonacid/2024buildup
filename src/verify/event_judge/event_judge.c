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
		else if((type==TYPE(PLC_ENGINEER))&&(subtype==SUBTYPE(PLC_ENGINEER,LOGIC_RETURN)))
		{
			ret=proc_code_upload_judge(sub_proc,recv_msg);
		}
	}
	return 0;
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
	score_event->name = dup_str("code_upload",0);
	
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
