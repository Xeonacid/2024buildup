#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <sys/time.h>
#include <string.h>

#include "data_type.h"
#include "cube.h"
#include "cube_define.h"
#include "cube_record.h"

#include "user_define.h"
#include "login_user.h"
// add para lib_include

char Buf[DIGEST_SIZE * 4];
int login_mode = 2;

int proc_logincmd_check(void *sub_proc, void *recv_msg);
int proc_returncmd_check(void *sub_proc, void *recv_msg);

int proc_login_request(void *sub_proc, void *recv_msg);
int proc_logout_request(void *sub_proc, void *recv_msg);

int proc_login_result(void *sub_proc, void *recv_msg);
int proc_logout_result(void *sub_proc, void *recv_msg);

int login_user_init(void *sub_proc, void *para)
{
	struct login_user_init_para *init_para = para;
	int ret;

	RECORD(USER_DEFINE, CLIENT_STATE) * client_state;
	// add yorself's module init func here
	client_state = Dalloc0(sizeof(*client_state), NULL);
	if (client_state == NULL)
		return -ENOMEM;

	Memset(client_state, 0, sizeof(*client_state));
	client_state->curr_state = WAIT;
	proc_share_data_setpointer(client_state);
	if (init_para == NULL)
		login_mode = 2;
	else
	{
		login_mode = init_para->mode;
		if ((login_mode < 0) || (login_mode > 2))
			login_mode = 0;
	}
	return 0;
}

int login_user_start(void *sub_proc, void *para)
{
	int ret;
	void *recv_msg;
	int type;
	int subtype;
	RECORD(USER_DEFINE, CLIENT_STATE) * client_state;
	// add yorself's module exec func here
	while (1)
	{
		usleep(time_val.tv_usec);
		ret = ex_module_recvmsg(sub_proc, &recv_msg);
		if (ret < 0)
			continue;
		if (recv_msg == NULL)
			continue;
		type = message_get_type(recv_msg);
		subtype = message_get_subtype(recv_msg);
		if (!memdb_find_recordtype(type, subtype))
		{
			printf("message format (%d %d) is not registered!\n",
				   message_get_type(recv_msg), message_get_subtype(recv_msg));
			continue;
		}

		if ((type == TYPE(GENERAL_RETURN)) && (subtype == SUBTYPE(GENERAL_RETURN, STRING)))
		{
			ret = proc_logincmd_check(sub_proc, recv_msg);
		}
		if ((type == TYPE(USER_DEFINE)) && (subtype == SUBTYPE(USER_DEFINE, RETURN)))
		{
			ret = proc_returncmd_check(sub_proc, recv_msg);
		}
	}
	return 0;
}

int proc_logincmd_check(void *sub_proc, void *recv_msg)
{
	int ret;
	RECORD(GENERAL_RETURN, STRING) * login_input;
	RECORD(USER_DEFINE, CLIENT_STATE) * client_state;
	void *new_msg;
	int offset;
	int type;
	int subtype;
	ret = message_get_record(recv_msg, &login_input, 0);

	if (ret < 0)
		return ret;

	type = message_get_type(recv_msg);
	subtype = message_get_subtype(recv_msg);

	if (Strcmp(login_input->name, "login") == 0)
	{
		if (login_mode == 0)
		{
			ret = proc_login_plainlogin(sub_proc, recv_msg);
		}
		// mode 1: sm3 hash passwd login
		else if (login_mode == 1)
		{
			ret = proc_login_hashlogin(sub_proc, recv_msg);
		}
		// mode 2: challenge_response  hash login
		else if (login_mode == 2)
		{
			client_state = proc_share_data_getpointer();
			if (client_state->curr_state == WAIT)
			{
				ret = proc_login_request(sub_proc, recv_msg);
			}
		}
	}
	else if (Strcmp(login_input->name, "logout") == 0)
	{
		return proc_logout_request(sub_proc, recv_msg);
	}
	else
		return -EINVAL;
}

int proc_returncmd_check(void *sub_proc, void *recv_msg)
{
	int ret = 0;

	RECORD(USER_DEFINE, RETURN) * return_record;
	RECORD(USER_DEFINE, CLIENT_STATE) * client_state;

	ret = message_get_record(recv_msg, &return_record, 0);

	client_state = proc_share_data_getpointer();

	if (return_record->return_code == LOGOUT_SUCCEED)
	{
		return proc_logout_result(sub_proc, recv_msg);
	}

	if (client_state->curr_state == REQUEST)
	{
		return proc_login_response(sub_proc, recv_msg);
	}
	else if (client_state->curr_state == RESPONSE)
	{
		return proc_login_result(sub_proc, recv_msg);
	}
	else if (client_state->curr_state == ERROR)
	{
		client_state->curr_state = WAIT;
	}
	else
	{
		return proc_login_result(sub_proc, recv_msg);
	}

	return ret;
}

int proc_login_plainlogin(void *sub_proc, void *recv_msg)
{
	int ret;
	RECORD(USER_DEFINE, CLIENT_STATE) * client_state;
	RECORD(USER_DEFINE, LOGIN) * login_info;
	RECORD(GENERAL_RETURN, STRING) * login_input;
	void *new_msg;

	ret = message_get_record(recv_msg, &login_input, 0);
	if (ret < 0)
		return ret;

	client_state = proc_share_data_getpointer();
	Memset(client_state->nonce, 0, DIGEST_SIZE);

	int offset;
	BYTE passbuf[DIGEST_SIZE];
	Memset(passbuf, 0, DIGEST_SIZE);

	login_info = Talloc0(sizeof(*login_info));
	if (login_info == NULL)
		return -ENOMEM;
	offset = Getfiledfromstr(passbuf, login_input->return_value, ':', DIGEST_SIZE);
	if (offset < 0)
		return -EINVAL;

	offset = Getfiledfromstr(passbuf, login_input->return_value, ':', DIGEST_SIZE);
	if (offset < 0)
		return -EINVAL;
	login_info->user_name = dup_str(passbuf, DIGEST_SIZE);

	// compute passwd
	Memset(passbuf, 0, DIGEST_SIZE);
	print_cubeaudit("login_user: get username %s", passbuf);

	client_state->user_name = dup_str(passbuf, DIGEST_SIZE);

	login_info->user_name = dup_str(client_state->user_name, DIGEST_SIZE);
	Memset(login_info->passwd, 0, DIGEST_SIZE);
	Strncpy(login_info->passwd, login_input->return_value + offset + 1, DIGEST_SIZE);

	ret = proc_share_data_getvalue("uuid", login_info->machine_uuid);
	ret = proc_share_data_getvalue("proc_name", login_info->proc_name);

	new_msg = message_create(TYPE_PAIR(USER_DEFINE, LOGIN), recv_msg);
	if (new_msg == NULL)
		return -EINVAL;
	ret = message_add_record(new_msg, login_info);
	if (ret < 0)
		return ret;

	ret = ex_module_sendmsg(sub_proc, new_msg);

	if (ret >= 0)
		client_state->curr_state = WAIT;
	proc_share_data_setpointer(client_state);
	return ret;
}

int proc_login_hashlogin(void *sub_proc, void *recv_msg)
{

	int ret;
	RECORD(USER_DEFINE, CLIENT_STATE) * client_state;
	RECORD(USER_DEFINE, LOGIN) * login_info;
	RECORD(GENERAL_RETURN, STRING) * login_input;
	void *new_msg;

	ret = message_get_record(recv_msg, &login_input, 0);
	if (ret < 0)
		return ret;

	client_state = proc_share_data_getpointer();
	Memset(client_state->nonce, 0, DIGEST_SIZE);
	int offset;
	BYTE passbuf[DIGEST_SIZE];
	Memset(passbuf, 0, DIGEST_SIZE);

	login_info = Talloc0(sizeof(*login_info));
	if (login_info == NULL)
		return -ENOMEM;
	offset = Getfiledfromstr(passbuf, login_input->return_value, ':', DIGEST_SIZE);
	if (offset < 0)
		return -EINVAL;

	client_state->user_name = dup_str(passbuf, DIGEST_SIZE);

	login_info->user_name = dup_str(client_state->user_name, DIGEST_SIZE);
	Memset(login_info->passwd, 0, DIGEST_SIZE);
	Strncpy(client_state->nonce, login_input->return_value + offset + 1, DIGEST_SIZE);

	calculate_context_sm3(client_state->nonce, DIGEST_SIZE, login_info->passwd);

	ret = proc_share_data_getvalue("uuid", login_info->machine_uuid);
	ret = proc_share_data_getvalue("proc_name", login_info->proc_name);

	new_msg = message_create(TYPE_PAIR(USER_DEFINE, LOGIN), recv_msg);
	if (new_msg == NULL)
		return -EINVAL;
	ret = message_add_record(new_msg, login_info);
	if (ret < 0)
		return ret;

	ret = ex_module_sendmsg(sub_proc, new_msg);

	if (ret >= 0)
		client_state->curr_state = WAIT;
	proc_share_data_setpointer(client_state);
	return ret;
}

int proc_login_request(void *sub_proc, void *recv_msg)
{
	int ret;
	RECORD(USER_DEFINE, CLIENT_STATE) * client_state;
	RECORD(USER_DEFINE, LOGIN) * login_info;
	RECORD(GENERAL_RETURN, STRING) * login_input;
	void *new_msg;

	ret = message_get_record(recv_msg, &login_input, 0);
	if (ret < 0)
		return ret;

	client_state = proc_share_data_getpointer();
	Memset(client_state->nonce, 0, DIGEST_SIZE);

	int offset;
	BYTE passbuf[DIGEST_SIZE];
	Memset(passbuf, 0, DIGEST_SIZE);

	login_info = Talloc0(sizeof(*login_info));
	if (login_info == NULL)
		return -ENOMEM;
	offset = Getfiledfromstr(passbuf, login_input->return_value, ':', DIGEST_SIZE);
	if (offset < 0)
		return -EINVAL;

	client_state->user_name = dup_str(passbuf, DIGEST_SIZE);

	login_info->user_name = dup_str(client_state->user_name, DIGEST_SIZE);
	Memset(login_info->passwd, 0, DIGEST_SIZE);
	Strncpy(client_state->nonce, login_input->return_value + offset + 1, DIGEST_SIZE);

	ret = proc_share_data_getvalue("uuid", login_info->machine_uuid);
	ret = proc_share_data_getvalue("proc_name", login_info->proc_name);

	new_msg = message_create(TYPE_PAIR(USER_DEFINE, LOGIN), recv_msg);
	if (new_msg == NULL)
		return -EINVAL;
	ret = message_add_record(new_msg, login_info);
	if (ret < 0)
		return ret;

	ret = ex_module_sendmsg(sub_proc, new_msg);

	if (ret >= 0)
		client_state->curr_state = REQUEST;
	proc_share_data_setpointer(client_state);
	return ret;
}

int proc_login_response(void *sub_proc, void *recv_msg)
{
	int ret;
	RECORD(USER_DEFINE, CLIENT_STATE) * client_state;
	RECORD(USER_DEFINE, LOGIN) * login_info;
	RECORD(USER_DEFINE, RETURN) * return_info;
	void *new_msg;
	client_state = proc_share_data_getpointer();
	if (client_state == NULL)
		return -EINVAL;
	ret = message_get_record(recv_msg, &return_info, 0);
	if (ret < 0)
		return ret;
	if (return_info->return_code != CHALLENGE)
	{
		ret = ex_module_sendmsg(sub_proc, recv_msg);
		client_state->curr_state = ERROR;
		return 0;
	}

	login_info = Talloc0(sizeof(*login_info));
	if (login_info == NULL)
		return -ENOMEM;
	client_state = proc_share_data_getpointer();

	login_info->user_name = dup_str(client_state->user_name, 0);

	Memset(login_info->passwd, 0, DIGEST_SIZE);
	ret = proc_share_data_getvalue("uuid", login_info->machine_uuid);
	ret = proc_share_data_getvalue("proc_name", login_info->proc_name);

	Memcpy(Buf, client_state->nonce, DIGEST_SIZE);
	Memcpy(Buf + DIGEST_SIZE, return_info->nonce, DIGEST_SIZE);
	calculate_context_sm3(Buf, DIGEST_SIZE * 2, login_info->passwd);

	//	new_msg=message_create(TYPE_PAIR(USER_DEFINE,LOGIN),NULL);

	new_msg = message_create(TYPE_PAIR(USER_DEFINE, LOGIN), recv_msg);
	if (new_msg == NULL)
		return -EINVAL;
	ret = message_add_record(new_msg, login_info);
	if (ret < 0)
		return ret;
	ret = ex_module_sendmsg(sub_proc, new_msg);

	if (ret >= 0)
		client_state->curr_state = RESPONSE;
	proc_share_data_setpointer(client_state);

	return ret;
}

int proc_logout_request(void *sub_proc, void *recv_msg)
{
	int ret;

	BYTE passbuf[DIGEST_SIZE];
	RECORD(USER_DEFINE, CLIENT_STATE) * client_state;
	RECORD(USER_DEFINE, LOGOUT) * logout_info;
	RECORD(GENERAL_RETURN, STRING) * input;
	void *new_msg;

	ret = message_get_record(recv_msg, &input, 0);
	if (ret < 0)
	{
		return ret;
	}

	logout_info = Talloc0(sizeof(*logout_info));

	if (logout_info == NULL)
	{
		return -ENOMEM;
	}

	int offset = Getfiledfromstr(passbuf, input->return_value, '\n', DIGEST_SIZE);

	logout_info->user_name = dup_str(passbuf, DIGEST_SIZE);

	new_msg = message_create(TYPE_PAIR(USER_DEFINE, LOGOUT), recv_msg);

	if (new_msg == NULL)
	{
		return -EINVAL;
	}

	ret = message_add_record(new_msg, logout_info);
	if (ret < 0)
	{
		return ret;
	}
	ret = ex_module_sendmsg(sub_proc, new_msg);

	return ret;
}

int proc_login_result(void *sub_proc, void *recv_msg)
{
	int ret;
	RECORD(USER_DEFINE, CLIENT_STATE) * client_state;
	RECORD(USER_DEFINE, RETURN) * return_info;
	void *new_msg;

	ret = message_get_record(recv_msg, &return_info, 0);
	if (ret < 0)
		return ret;

	client_state = proc_share_data_getpointer();

	if (return_info->return_code == SUCCEED)
	{
		print_cubeaudit("user %s login succeed!", client_state->user_name);
		client_state->curr_state = LOGIN;
	}
	else if (return_info->return_code == NOAUTH)
	{
		print_cubeaudit("user %s connect to server succeed!", client_state->user_name);
		client_state->curr_state = CONNECT;
	}
	else
	{
		print_cubeerr("user %s login failed!", client_state->user_name);
		client_state->curr_state = ERROR;
	}

	proc_share_data_setvalue("user_name", client_state->user_name);

	ret = ex_module_sendmsg(sub_proc, recv_msg);

	proc_share_data_setpointer(client_state);

	return ret;
}
/* 退出登录逻辑 */

int proc_logout_result(void *sub_proc, void *recv_msg)
{
	int ret;
	RECORD(USER_DEFINE, CLIENT_STATE) * client_state;
	RECORD(USER_DEFINE, RETURN) * return_info;
	void *new_msg;

	ret = message_get_record(recv_msg, &return_info, 0);
	if (ret < 0)
		return ret;

	// get client state
	client_state = proc_share_data_getpointer();

	if (return_info->return_code == LOGOUT_SUCCEED)
	{
		client_state->curr_state = WAIT;
		print_cubeaudit("user %s logout succeed!", client_state->user_name);
	} /* 修复添加else情况匹配*/
	else
	{
		print_cubeerr("user %s logout failed!", client_state->user_name);
	}

	ret = ex_module_sendmsg(sub_proc, recv_msg);

	proc_share_data_setpointer(client_state);

	return ret;
}

