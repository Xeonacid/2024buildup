#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <sys/time.h>
 
#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "memdb.h"
#include "message.h"
#include "ex_module.h"
#include "sys_func.h"
#include "user_define.h"
#include "login_server.h"


int proc_login_login(void * sub_proc,void * recv_msg);
int proc_logout_result(void * sub_proc,void * recv_msg);

// add para lib_include
BYTE Buf[DIGEST_SIZE*4];
BYTE Empty_Password [DIGEST_SIZE];

int login_server_challenge(void * sub_proc,void * login_data,void * msg);
int login_server_verify(void * sub_proc,void * login_data,void * msg);

int login_server_init(void * sub_proc, void * para)
{
	int ret;
	// add yorself's module init func here
    Memset(Empty_Password,0,DIGEST_SIZE);
	return 0;
}
int login_server_start(void * sub_proc, void * para)
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
		if((type==TYPE(USER_DEFINE))&&(subtype==SUBTYPE(USER_DEFINE,LOGIN)))
		{
			ret=proc_login_login(sub_proc,recv_msg);
		}
		/*截获当前退出的消息类型，执行对应的 logout 功能*/ 
		if((type==TYPE(USER_DEFINE))&&(subtype==SUBTYPE(USER_DEFINE,LOGOUT)))
		{
			ret=proc_logout_result(sub_proc,recv_msg);
		}
	}
	return 0;
}

int proc_login_login(void * sub_proc,void * recv_msg)
{
	int ret;
	RECORD(USER_DEFINE,SERVER_STATE) * user_state;
    // user login state
	RECORD(USER_DEFINE,LOGIN) * login_info;
    // user login info
	RECORD(USER_DEFINE,RETURN) * return_info;
    // user login return info
	void * new_msg;
	
	ret=message_get_record(recv_msg,&login_info,0);
	if(ret<0)
		return ret;

	return_info=Talloc0(sizeof(*return_info));
	if(return_info==NULL)
		return -ENOMEM;

	DB_RECORD * db_record;

	db_record=memdb_find_first(TYPE_PAIR(USER_DEFINE,SERVER_STATE),"user_name",login_info->user_name);
	if(db_record==NULL)
	{
		return_info->return_code=NOUSER;
		return_info->return_info=dup_str("no such user!\n",0);
	}
	else
	{
		user_state=db_record->record;
		if((user_state->curr_state==0) || (user_state->curr_state == SUCCEED))
		{
            // judge if the password is empty
            // if password is empty, enter the CHALLENGE mode
            if (Memcmp(login_info->passwd,Empty_Password,DIGEST_SIZE)==0)
            {
			    user_state->curr_state=CHALLENGE;
			    Memcpy(user_state->proc_name,login_info->proc_name,DIGEST_SIZE);
			    Memcpy(user_state->node_uuid,login_info->machine_uuid,DIGEST_SIZE);
			    RAND_bytes(user_state->nonce,DIGEST_SIZE);
			    return_info->return_code=CHALLENGE;
			    Memcpy(return_info->nonce,user_state->nonce,DIGEST_SIZE);
            }

            // if password is not empty, enter the normal login mode
            else
            {
                // get temp
                Memset(Buf,0,DIGEST_SIZE);
                Strncpy(Buf,user_state->passwd,DIGEST_SIZE);
                
			    calculate_context_sm3(Buf,DIGEST_SIZE,Buf+DIGEST_SIZE);
		
			    if(Memcmp(Buf+DIGEST_SIZE,login_info->passwd,DIGEST_SIZE)==0)
			    {
				    return_info->return_code=SUCCEED;
				    return_info->return_info=dup_str("login succeed!\n",0);
			    }
			    else
			    {
				    return_info->return_code=AUTHFAIL;
				    return_info->return_info=dup_str("password error!\n",0);
			    }
		    }
		    user_state->curr_state=return_info->return_code;
		    memdb_store(user_state,TYPE_PAIR(USER_DEFINE,SERVER_STATE),NULL);
	    }
		else if (user_state->curr_state ==CHALLENGE)
		{
			// enter the verify mode
			Memset(Buf,0,DIGEST_SIZE);
			Strncpy(Buf,user_state->passwd,DIGEST_SIZE);
			Memcpy(Buf+DIGEST_SIZE,user_state->nonce,DIGEST_SIZE);
			calculate_context_sm3(Buf,DIGEST_SIZE*2,Buf+DIGEST_SIZE*2);
		
			if(Memcmp(Buf+DIGEST_SIZE*2,login_info->passwd,DIGEST_SIZE)==0)
			{
				return_info->return_code=SUCCEED;
				return_info->return_info=dup_str("login succeed!\n",0);
			}
			else
			{
				return_info->return_code=AUTHFAIL;
				return_info->return_info=dup_str("password error!\n",0);
			}
		}
		user_state->curr_state=return_info->return_code;
		memdb_store(user_state,TYPE_PAIR(USER_DEFINE,SERVER_STATE),NULL);
	}
				
	new_msg=message_create(TYPE_PAIR(USER_DEFINE,RETURN),recv_msg);	
	if(new_msg==NULL)
		return -EINVAL;
	ret=message_add_record(new_msg,return_info);
	if(ret<0)
		return ret;
	
	ret=ex_module_sendmsg(sub_proc,new_msg);
    // verify code start

	new_msg=message_create(TYPE_PAIR(USER_DEFINE,SERVER_STATE),NULL);	
	if(new_msg==NULL)
		return -EINVAL;
	ret=message_add_record(new_msg,user_state);
	if(ret<0)
		return ret;
	
	ret=ex_module_sendmsg(sub_proc,new_msg);
    // verify code end

	return ret;
}
/* 退出登录逻辑 */ 
int proc_logout_result(void * sub_proc,void * recv_msg)
{
	int ret;
	RECORD(USER_DEFINE,SERVER_STATE) * user_state;
	RECORD(USER_DEFINE,LOGOUT) * logout_info;
	RECORD(USER_DEFINE,RETURN) * return_info;
	void * new_msg;

	ret=message_get_record(recv_msg,&logout_info,0);
	if(ret<0)
		return ret;

	return_info=Talloc0(sizeof(*return_info));
	if(return_info==NULL)
		return -ENOMEM;

	DB_RECORD * db_record;

	db_record=memdb_find_first(TYPE_PAIR(USER_DEFINE,SERVER_STATE),"user_name",logout_info->user_name);

	if(db_record == NULL)
	{
		return_info->return_code=NOUSER;
		return_info->return_info=dup_str("no such user!\n",0);
	}else{

		user_state=db_record->record;
        user_state->curr_state = WAIT;
        memdb_store(user_state,TYPE_PAIR(USER_DEFINE,SERVER_STATE),NULL);
		
		int user_len = Strlen(logout_info->user_name);
		char* return_msg = Talloc0(user_len + 20);
		Memcpy(return_msg,logout_info->user_name,user_len);
		return_msg = Strcat(return_msg," logout succeed!\n");

		return_info->return_code=LOGOUT_SUCCEED;
		return_info->return_info=return_msg;

	}

	new_msg=message_create(TYPE_PAIR(USER_DEFINE,RETURN),recv_msg);	
	if(new_msg==NULL)
		return -EINVAL;
	ret=message_add_record(new_msg,return_info);
	if(ret<0)
		return ret;
	
	ret=ex_module_sendmsg(sub_proc,new_msg);
	return ret;
}

