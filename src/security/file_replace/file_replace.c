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

#include "data_type.h"
#include "cube.h"
#include "cube_define.h"
#include "cube_record.h"

#include "plc_emu.h"

extern struct timeval time_val={0,50*1000};

int file_replace_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
	return 0;
}

int file_replace_start(void * sub_proc,void * para)
{
	int ret;
	void * recv_msg;
	int i;
	int type;
	int subtype;


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
			ex_module_sendmsg(sub_proc,recv_msg);
			continue;
		}
		if((type==DTYPE_MESSAGE)&&(subtype==SUBTYPE_CTRL_MSG))
		{
			ret=proc_ctrl_message(sub_proc,recv_msg);
			if(ret==MSG_CTRL_EXIT)
				return MSG_CTRL_EXIT;
		}
		else if((type==TYPE(PLC_ENGINEER)&&(subtype==SUBTYPE(PLC_ENGINEER,LOGIC_UPLOAD))))
		{
			ret= bin_file_replace(sub_proc,recv_msg);
		}
	}
	return 0;
};

static unsigned char Buf[DIGEST_SIZE*32];

int dup_file(char * oldfile, char * newfile)
{
	int ret;
    int len;

    int fd1,fd2;
    int size=DIGEST_SIZE*32;

    fd1 = open(oldfile,O_RDONLY);
    if(fd1<0)
        return fd1;
    
    fd2=open(newfile,O_WRONLY|O_CREAT|O_TRUNC);
    if(fd2<0)
    {
        close(fd1);
        return fd2;
    }

    len = read(fd1,Buf,size);
    while(len>0)
    {
        ret=write(fd2,Buf,len);
        if(ret!=len)
            return -EINVAL;
        if(len<size)
            break;
        len = read(fd1,Buf,size);
    }
    
    close(fd1);
    close(fd2);
    return 0;
}

int bin_file_replace(void * sub_proc,void * recv_msg)
{
	int type;
	int subtype;
	int i;
	int ret;

	RECORD(PLC_ENGINEER,LOGIC_UPLOAD) * bin_upload;

	ret=message_get_record(recv_msg,&bin_upload,0);
	if(ret<0)
		return ret;
	// 在这里添加文件替换代码
	dup_file("/root/2024buildup/src/logic/hack_logic/libthermostat_logic.so", "/root/2024buildup/src/logic/thermostat_logic/libthermostat_logic.so");

	//文件替换代码结束
	ex_module_sendmsg(sub_proc,recv_msg);

	return ret;
}

int proc_ctrl_message(void * sub_proc,void * message)
{
	int ret;
	int i=0;
	printf("begin proc echo \n");

	struct ctrl_message * ctrl_msg;

	
	ret=message_get_record(message,&ctrl_msg,i++);
	if(ret<0)
		return ret;
	if(ctrl_msg!=NULL)
	{
		ret=ctrl_msg->ctrl; 
	}

	ex_module_sendmsg(sub_proc,message);

	return ret;
}
