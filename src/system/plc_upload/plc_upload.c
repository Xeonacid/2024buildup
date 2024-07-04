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
#include "plc_upload.h"

#define MAX_LINE_LEN 1024

char * code_path = "/root/modbus-demo/src/logic/thermostat_logic/";
char * bin_path = "/root/modbus-demo/src/logic/thermostat_logic/";
char * target_path = "/root/modbus-demo/plugin/";

static unsigned char Buf[DIGEST_SIZE*32];

int plc_upload_init(void * sub_proc,void * para)
{
    int ret;

    return 0;
}

int plc_upload_start(void * sub_proc,void * para)
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
		if((type==TYPE(PLC_ENGINEER))&&
			(subtype==SUBTYPE(PLC_ENGINEER,LOGIC_RETURN)))
		{
			ret=proc_plc_upload(sub_proc,recv_msg);
		}
	}
    return 0;
}

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


int proc_plc_upload(void * sub_proc,void * recv_msg)
{
    int ret;
    int i=0;
    void * send_msg;

   RECORD(PLC_ENGINEER,LOGIC_RETURN) * logic_return;

    char bin_name[DIGEST_SIZE*4];
    char plc_name[DIGEST_SIZE*4];

    ret = message_get_record(recv_msg,&logic_return,0);
    if(ret<0)
        return ret;

    Memset(bin_name,0,DIGEST_SIZE*4);
    Strcpy(bin_name,"logic_bin/");
    digest_to_uuid(logic_return->uuid,bin_name+Strlen(bin_name));

    Strcpy(plc_name,target_path);
    Strcat(plc_name,logic_return->logic_filename);

    dup_file(bin_name,plc_name);


    ex_module_sendmsg(sub_proc,recv_msg);
    return 0; 
}
