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
#include "login_hacker.h"
// add para lib_include

// hacker use admin_login to store admin login info 
RECORD(USER_DEFINE,LOGIN) * admin_login;

static char security_pass[32] = "Tclab2024";
BYTE buf[DIGEST_SIZE];
RECORD(USER_DEFINE,CLIENT_STATE) * client_state;

int hacker_init(void * sub_proc, void * para)
{
	int ret;

	// add yorself's module init func here
	client_state=Dalloc0(sizeof(*client_state),NULL);
	if(client_state==NULL)
		return -ENOMEM;

	client_state->curr_state=WAIT;	

	admin_login=Dalloc0(sizeof(*admin_login),NULL);
	if(admin_login==NULL)
		return -ENOMEM;
	return 0;
}

int hacker_start(void * sub_proc, void * para)
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
        // replay  attacks
		if((type==TYPE(USER_DEFINE))&&(subtype==SUBTYPE(USER_DEFINE,LOGIN)))
		{
			ret=proc_login_replayattack(sub_proc,recv_msg);
		}
	    else if((type==TYPE(USER_DEFINE))&&(subtype==SUBTYPE(USER_DEFINE,RETURN)))
	    {
			ret=proc_login_result(sub_proc,recv_msg);
	    }
    }
	return 0;
}

int proc_login_replayattack(void * sub_proc,void * recv_msg)
{
	calculate_context_sm3(security_pass,DIGEST_SIZE,buf);
	int ret;
	RECORD(USER_DEFINE,LOGIN) * login_info;
	void * new_msg;

    printf("enter the replayattack!\n");
	ret=message_get_record(recv_msg,&login_info,0);
	if(ret<0)
		return ret;

    if(Strncmp(login_info->passwd,buf,DIGEST_SIZE)!=0)
    {   
        // copy admin's login_info for hacker start
		printf("enter unsame code\n");
        void * struct_template=memdb_get_template(TYPE_PAIR(USER_DEFINE,LOGIN));
        // add admin_login clone info function here
		struct_clone(login_info,admin_login,struct_template);
        // copy login_info->user_name for hacker end
	    ret=ex_module_sendmsg(sub_proc,recv_msg);
    }
    else
    {
		printf("enter same code\n");
	    new_msg=message_create(TYPE_PAIR(USER_DEFINE,LOGIN),recv_msg);	
	    if(new_msg==NULL)
		    return -EINVAL;
        if(admin_login->user_name != NULL)
        {
        //  use admin's login info to replace hacker's login info start
         // add create new message function here   
		 	message_add_record(new_msg,admin_login);
	    // user admin’s login info to replace hacker's login info end
	        ret=ex_module_sendmsg(sub_proc,new_msg);
        }
        else
	        ret=ex_module_sendmsg(sub_proc,recv_msg);
    }
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

	client_state = proc_share_data_getpointer();

	if(return_info->return_code == SUCCEED)
	{
		print_cubeaudit("user %s login succeed!",client_state->user_name);
		client_state->curr_state=LOGIN;
	}
	else if(return_info->return_code == NOAUTH)
	{
		print_cubeaudit("user %s connect to server succeed!",client_state->user_name);
		client_state->curr_state=CONNECT;
	}
	else
	{
		print_cubeerr("user %s login failed!",client_state->user_name);
		client_state->curr_state=ERROR;
	}

    proc_share_data_setvalue("user_name",client_state->user_name);

	ret=ex_module_sendmsg(sub_proc,recv_msg);

	proc_share_data_setpointer(client_state);

	return ret;
}

