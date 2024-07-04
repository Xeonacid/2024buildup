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
#include "logic_upload.h"

#define MAX_LINE_LEN 1024

char * code_path = "/root/modbus-demo/src/logic/thermostat_logic/";
char * bin_path = "/root/modbus-demo/src/logic/thermostat_logic/";
char * target_path = "/root/modbus-demo/plugin/";

static unsigned char Buf[DIGEST_SIZE*32];

int logic_upload_init(void * sub_proc,void * para)
{
    int ret;
    if(access("logic_code",F_OK)==-1)
        system("mkdir logic_code");
        
    if(access("logic_bin",F_OK)==-1)
        system("mkdir logic_bin");

    return 0;
}

int logic_upload_start(void * sub_proc,void * para)
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
			(subtype==SUBTYPE(PLC_ENGINEER,LOGIC_UPLOAD)))
		{
			ret=proc_logic_upload(sub_proc,recv_msg);
		}
	}
    return 0;
}

int get_short_uuidstr(int len,BYTE * digest,char * uuidstr)
{
	int ret;
	char uuid_buf[DIGEST_SIZE*2];
	
	if(len<0)
		return -EINVAL;
	if(len>32)
		len=32;
	ret=len*2;
	digest_to_uuid(digest,uuid_buf);
	Memcpy(uuidstr,uuid_buf,ret);
	uuidstr[ret]=0;
	return ret;
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


int dup_uuidname(char * name,int len,BYTE * digest,char * newfilename)
{
	int i;
	int lastsplit;
	int offset;
	int ret;
	char uuidstr[DIGEST_SIZE*2];
	char filename[DIGEST_SIZE*4];

	if((len<0)||(len>32))
		return -EINVAL;
	if(len==0)
		len=DIGEST_SIZE;

	ret=calculate_sm3(name,digest);
	if(ret<0)
		return ret;

	len=get_short_uuidstr(len,digest,uuidstr);

	Strncat(newfilename,uuidstr,DIGEST_SIZE*3);
	
	ret=dup_file(name,newfilename);
	if(ret<0)
		return ret;
	return 1;	
} 

int proc_logic_upload(void * sub_proc,void * recv_msg)
{
    int ret;
    int i=0;
    void * send_msg;
    void * file_msg;
    time_t curr_time;

   RECORD(PLC_ENGINEER,LOGIC_UPLOAD) * logic_upload;
   RECORD(PLC_ENGINEER,LOGIC_CODE) * logic_code;
   RECORD(PLC_ENGINEER,LOGIC_BIN) * logic_bin;
   RECORD(FILE_TRANS,REQUEST) * file_req;

    char full_filename[DIGEST_SIZE*4];
    char new_filename[DIGEST_SIZE*4];
    BYTE file_uuid[DIGEST_SIZE];    

    ret = message_get_record(recv_msg,&logic_upload,0);
    if(ret<0)
        return ret;

    if(logic_upload->type  == FILE_CODE)
    {
        // compute file uuid
        Strcpy(full_filename,code_path);
        Strcat(full_filename,logic_upload->logic_filename);
        Strcpy(new_filename,"logic_code/");
        ret = dup_uuidname(full_filename,DIGEST_SIZE,logic_upload->uuid,new_filename);
        if(ret<0)
        {
            printf("convert file fail! %d\n",ret);
             return ret;
        }
        logic_upload->time = time(&curr_time);

        // fill the logic code struct and store it
        logic_code = Talloc0(sizeof(*logic_code));
        if(logic_code == NULL)
            return -ENOMEM;
        logic_code->plc_devname = dup_str(logic_upload->plc_devname,DIGEST_SIZE);
        logic_code->logic_filename = dup_str(logic_upload->logic_filename,DIGEST_SIZE);
        Memcpy(logic_code->code_uuid,logic_upload->uuid,DIGEST_SIZE);
        logic_code->author = dup_str(logic_upload->author,DIGEST_SIZE);
        logic_code->time=logic_upload->time;

        memdb_store(logic_code,TYPE_PAIR(PLC_ENGINEER,LOGIC_CODE),NULL);

        // create file_trans command
        file_req=Talloc0(sizeof(*file_req));
        if(file_req == NULL)
            return -ENOMEM;
        Memcpy(file_req->uuid,logic_upload->uuid,DIGEST_SIZE);
        file_req->filename = dup_str(new_filename,0);
        file_req->requestor = dup_str("logic_upload",0);

        file_msg = message_create(TYPE_PAIR(FILE_TRANS,REQUEST),recv_msg);
        if(file_msg==NULL)
            return -EINVAL;
        message_add_record(file_msg,file_req);
        ex_module_sendmsg(sub_proc,file_msg);

        // create plc upload msg

        send_msg = message_create(TYPE_PAIR(PLC_ENGINEER,LOGIC_UPLOAD),recv_msg);
        if(send_msg==NULL)
            return -EINVAL;
        message_add_record(send_msg,logic_upload);
        message_add_expand_data(send_msg,TYPE_PAIR(PLC_ENGINEER,LOGIC_CODE),logic_code);

        ex_module_sendmsg(sub_proc,send_msg);

    }
    else if(logic_upload->type == FILE_LOGIC)
    {
        // compute file uuid
        Strcpy(full_filename,bin_path);
        Strcat(full_filename,logic_upload->logic_filename);
        Strcpy(new_filename,"logic_bin/");
        ret = dup_uuidname(full_filename,DIGEST_SIZE,logic_upload->uuid,new_filename);
        if(ret<0)
             return ret;
        logic_upload->time = time(&curr_time);

        // fill the logic bin struct and store it
        logic_bin = Talloc0(sizeof(*logic_bin));
        if(logic_bin == NULL)
            return -ENOMEM;
        logic_bin->plc_devname = dup_str(logic_upload->plc_devname,DIGEST_SIZE);
        logic_bin->logic_filename = dup_str(logic_upload->logic_filename,DIGEST_SIZE);
        Memcpy(logic_bin->bin_uuid,logic_upload->uuid,DIGEST_SIZE);
        logic_bin->author = dup_str(logic_upload->author,DIGEST_SIZE);
        logic_bin->time=logic_upload->time;

        memdb_store(logic_bin,TYPE_PAIR(PLC_ENGINEER,LOGIC_BIN),NULL);

        // create file_trans command
        file_req=Talloc0(sizeof(*file_req));
        if(file_req == NULL)
            return -ENOMEM;
        Memcpy(file_req->uuid,logic_upload->uuid,DIGEST_SIZE);
        file_req->filename = dup_str(new_filename,0);
        file_req->requestor = dup_str("logic_upload",0);

        file_msg = message_create(TYPE_PAIR(FILE_TRANS,REQUEST),recv_msg);
        if(file_msg==NULL)
            return -EINVAL;
        message_add_record(file_msg,file_req);
        ex_module_sendmsg(sub_proc,file_msg);

        // create plc upload msg

        send_msg = message_create(TYPE_PAIR(PLC_ENGINEER,LOGIC_UPLOAD),recv_msg);
        if(send_msg==NULL)
            return -EINVAL;
        message_add_record(send_msg,logic_upload);
        message_add_expand_data(send_msg,TYPE_PAIR(PLC_ENGINEER,LOGIC_BIN),logic_bin);

        ex_module_sendmsg(sub_proc,send_msg);

    }
    else
        return -EINVAL;


    return 0; 
}
