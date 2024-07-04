#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <sys/time.h>
 
#include "data_type.h"
#include "cube.h"
#include "cube_define.h"
#include "cube_record.h"

#include "user_define.h"

#include "login_user.h"
// add para lib_include

char Buf[DIGEST_SIZE*4];

int login_user_init(void * sub_proc, void * para)
{
	int ret;
	// add yorself's module init func here
	RECORD(USER_DEFINE,CLIENT_STATE) * client_state;
	// add yorself's module init func here
	client_state=Dalloc0(sizeof(*client_state),NULL);
	if(client_state==NULL)
		return -ENOMEM;

	Memset(client_state,0,sizeof(*client_state));
	client_state->curr_state=WAIT;	
	proc_share_data_setpointer(client_state);	

	return 0;
}

int login_user_start(void * sub_proc, void * para)
{
	int ret;
	void * recv_msg;
	int type;
	int subtype;
	RECORD(USER_DEFINE,CLIENT_STATE) * client_state;
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
		if((type==TYPE(GENERAL_RETURN))&&(subtype==SUBTYPE(GENERAL_RETURN,STRING)))
		{
			ret=proc_login_request(sub_proc,recv_msg);
		}
		if((type==TYPE(USER_DEFINE))&&(subtype==SUBTYPE(USER_DEFINE,RETURN)))
		{
			ret=proc_login_result(sub_proc,recv_msg);
		}
	}
	return 0;
}

int proc_login_request(void * sub_proc,void * recv_msg)
{
	int ret;

    BYTE passbuf[DIGEST_SIZE];
	RECORD(USER_DEFINE,CLIENT_STATE) * client_state;
	RECORD(USER_DEFINE,LOGIN) * login_info;
	RECORD(GENERAL_RETURN,STRING) * login_input;
	void * new_msg;
    int offset;

	ret=message_get_record(recv_msg,&login_input,0);
	if(ret<0)
		return ret;
    // get name from login_input's name
    // the input info is user_name:role, while role can be SYSADMIN, USER,etc

    

	Memset(passbuf,0,DIGEST_SIZE);	
	login_info=Talloc0(sizeof(*login_info));
	if(login_info==NULL)
		return -ENOMEM;
	
    offset=Getfiledfromstr(passbuf,login_input->return_value,':',DIGEST_SIZE);
    if(offset<0)
        return -EINVAL;
	login_info->user_name=dup_str(passbuf,DIGEST_SIZE);
    
    // compute passwd
    Memset(passbuf,0,DIGEST_SIZE);

	Strncpy(passbuf,login_input->return_value+offset+1,DIGEST_SIZE);
    calculate_context_sm3(passbuf,DIGEST_SIZE,login_info->passwd);

    // get node_uuid
	ret=proc_share_data_getvalue("uuid",login_info->machine_uuid);


    // create client_state 
	client_state = proc_share_data_getpointer();
	client_state->user_name=dup_str(login_info->user_name,DIGEST_SIZE);
    client_state->curr_state=WAIT;

    // send login message
	new_msg=message_create(TYPE_PAIR(USER_DEFINE,LOGIN),recv_msg);	
	if(new_msg==NULL)
		return -EINVAL;
	ret=message_add_record(new_msg,login_info);
	if(ret<0)
		return ret;
	
	ret=ex_module_sendmsg(sub_proc,new_msg);

	if(ret >=0)
		client_state->curr_state=WAIT;
	proc_share_data_setpointer(client_state);	
	return ret;
}

int proc_login_result(void * sub_proc,void * recv_msg)
{
	int ret;
	RECORD(USER_DEFINE,CLIENT_STATE) * client_state;
	RECORD(USER_DEFINE,RETURN) * return_info;
	void * new_msg;
	
	ret=message_get_record(recv_msg,&return_info,0);
	if(ret<0)
		return ret;

    // get client state
	client_state = proc_share_data_getpointer();

	if(return_info->return_code == SUCCEED)
	{
		print_cubeaudit("user %s login succeed!",client_state->user_name);
		client_state->curr_state=LOGIN;
	}
	{
		print_cubeerr("user %s login failed!",client_state->user_name);
		client_state->curr_state=ERROR;
	}

    proc_share_data_setvalue("user_name",client_state->user_name);

	ret=ex_module_sendmsg(sub_proc,recv_msg);

	proc_share_data_setpointer(client_state);

	return ret;
}
