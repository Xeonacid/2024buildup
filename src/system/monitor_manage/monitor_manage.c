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
#include <sys/types.h>
#include <pwd.h>

#include "data_type.h"
#include "cube.h"
#include "cube_define.h"
#include "cube_record.h"
#include "file_struct.h"

#include "plc_emu.h"
#include "user_define.h"
#include "monitor_manage.h"

#define MAX_LINE_LEN 1024


int monitor_manage_init(void * sub_proc,void * para)
{
    int ret;

    return 0;
}

int monitor_manage_start(void * sub_proc,void * para)
{
    int ret = 0, len = 0, i = 0, j = 0;
    int rc = 0;

    int type,subtype;	
    void * recv_msg;

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
		if((type==TYPE(PLC_MONITOR))&&
			(subtype==SUBTYPE(PLC_MONITOR,PLC_CMD)))
		{
			ret=proc_monitor_manage(sub_proc,recv_msg);
		}
		else if((type==TYPE(PLC_MONITOR))&&
			(subtype==SUBTYPE(PLC_MONITOR,PLC_RETURN)))
		{
			ret=proc_monitor_return(sub_proc,recv_msg);
		}
	}
    return 0;
}

int proc_monitor_manage(void * sub_proc,void * recv_msg)
{
    int ret;
    int i=0;
    void * send_msg;
    void * file_msg;
    void * logic_msg;
    time_t curr_time;
    BYTE msg_uuid[DIGEST_SIZE];
    RECORD(PLC_MONITOR,PLC_CMD) * plc_cmd;

    ret = message_get_record(recv_msg,&plc_cmd,0);
    if(ret<0)
        return ret;

    plc_cmd->time = time(&curr_time);
    
    send_msg = message_create(TYPE_PAIR(PLC_MONITOR,PLC_CMD),recv_msg);
	if(send_msg == NULL)
			return -EINVAL;
	message_add_record(send_msg,plc_cmd);

    ex_module_sendmsg(sub_proc,send_msg);


    return 0; 
}

int proc_monitor_return(void * sub_proc,void * recv_msg)
{
    ex_module_sendmsg(sub_proc,recv_msg);

    return 0;
}
