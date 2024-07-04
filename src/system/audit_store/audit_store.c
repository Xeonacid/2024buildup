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

#include "user_define.h"
#include "plc_emu.h"
#include "modbus_cmd.h"
#include "audit_store.h"

#define MAX_LINE_LEN 1024


static unsigned char Buf[DIGEST_SIZE*32];

int audit_store_init(void * sub_proc,void * para)
{
    int ret;
    if(access("operator_audit",F_OK)==-1)
        system("mkdir operator_audit");
    if(access("device_audit",F_OK)==-1)
        system("mkdir device_audit");
    if(access("engineer_audit",F_OK)==-1)
        system("mkdir engineer_audit");
        

    return 0;
}

int audit_store_start(void * sub_proc,void * para)
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
		if((type==TYPE(PLC_OPERATOR))&&
			(subtype==SUBTYPE(PLC_OPERATOR,PLC_CMD)))
		{
			// store operator command audit	
			ret=proc_operator_audit_store(sub_proc,recv_msg);
		}
		else if((type==TYPE(PLC_OPERATOR))&&
			(subtype==SUBTYPE(PLC_OPERATOR,PLC_RETURN)))
		{
			// store return value
			ret=proc_device_audit_store(sub_proc,recv_msg);
		}
    }
    return 0;
}

char * build_audit_filename(char * dir_name,int start_time)
{
	char name_buf[DIGEST_SIZE*2];
	sprintf(name_buf,"%s/%d",dir_name,start_time);

	return dup_str(name_buf,DIGEST_SIZE*2);
}


char * _audit_find_file(char * dir_name,int check_time,int direction)
{
   DIR    *dir;
   struct dirent  *ptr;
   int max_time=0;
   int min_time=0;
   int file_time;
   char file_name[DIGEST_SIZE*2];
   if ((dir = opendir(dir_name)) == NULL) {
                print_cubeerr("can't find audit file dir");
                return -EINVAL;
   }


   while ((ptr = readdir(dir)) != NULL) {
          if (strcmp(ptr->d_name, ".") == 0
                 || strcmp(ptr->d_name, "..") == 0
                 || (ptr->d_type & DT_REG) == 0)
          	continue;
   	  file_time= Atoi(ptr->d_name,DIGEST_SIZE);	
	  if(direction==1) {   // find last file before check_time
		if(file_time < check_time)
			if(file_time>max_time)
				max_time=file_time;

	  }
	  else if(direction == -1){  // find first file after check_time;
		if(file_time > check_time)
		{
			if(min_time ==0)
			{
				min_time = file_time;
			}
			else
			{
				if(file_time<min_time)
					min_time=file_time;
			}
		}	

	  }
	  else
		return NULL;	  

	  if(direction == 1)
	  {
		  if(max_time == 0)
			  return NULL;
		  return build_audit_filename(dir_name,max_time);
	  }
	  if(min_time ==0)
		  return NULL;
	  return build_audit_filename(dir_name,min_time);
   }
}


char *  audit_find_last_file(char * dir_name,int check_time)
{

	return _audit_find_file(dir_name,check_time,1);
	
}

char *  audit_find_next_file(char * dir_name,int check_time)
{
	return _audit_find_file(dir_name,check_time,-1);
}

int proc_operator_audit_store(void * sub_proc,void * recv_msg)
{
    int ret;
    int i=0;
    void * send_msg;
    int fd;
   RECORD(PLC_OPERATOR,PLC_CMD) * plc_cmd;
   RECORD(PLC_MONITOR,PLC_AUDIT) * plc_audit;

   void * audit_template;
   void * cmd_template;
   int audit_head_size;
   char * audit_filename;
   char * dir_name = "operator_audit";
   char DataBuf[DIGEST_SIZE*16];
   off_t offset;

   audit_template = memdb_get_template(TYPE_PAIR(PLC_MONITOR,PLC_AUDIT));
   audit_head_size = struct_size(audit_template);

   ret = message_get_record(recv_msg,&plc_cmd,0);
   if(ret <0)
	   return ret;
   
   audit_filename = audit_find_last_file(dir_name,plc_cmd->time);
   if(audit_filename == NULL)
   {
	audit_filename=build_audit_filename(dir_name,plc_cmd->time);
	if(audit_filename==NULL)
		return -EINVAL;
	plc_audit =Talloc0(audit_head_size);
	if(plc_audit == NULL)
		return -ENOMEM;
	Strncpy(plc_audit->plc_devname,plc_cmd->plc_devname,DIGEST_SIZE);
	plc_audit->user_type = PLC_OPERATOR;
	plc_audit->start_time = plc_cmd->time;
	plc_audit->end_time = plc_cmd->time;
	plc_audit->event_num=1;

	ret = struct_2_blob(plc_audit,DataBuf,audit_template);
	if(ret<0)
		return ret;
	fd = open(audit_filename,O_WRONLY|O_CREAT);
	if(fd<0)
		return fd;
	write(fd,DataBuf,ret);

   }
   else
   {
	fd = open(audit_filename,O_RDONLY);
	if(fd<0)
		return fd;
	plc_audit =Talloc0(audit_head_size);
	if(plc_audit == NULL)
		return -ENOMEM;

	ret = read(fd,plc_audit,audit_head_size);
	if(ret<0)
		return ret;
	close(fd);	
	if(plc_audit->event_num >99)
	{
		audit_filename=build_audit_filename(dir_name,plc_cmd->time);
		if(audit_filename==NULL)
			return -EINVAL;
		plc_audit =Talloc0(audit_head_size);
		if(plc_audit == NULL)
			return -ENOMEM;
		Strncpy(plc_audit->plc_devname,plc_cmd->plc_devname,DIGEST_SIZE);
		plc_audit->user_type = PLC_OPERATOR;
		plc_audit->start_time = plc_cmd->time;
		plc_audit->end_time = plc_cmd->time;
		plc_audit->event_num=1;
	}
	else
	{

		plc_audit->end_time = plc_cmd->time;
		plc_audit->event_num++;
	}
	fd = open(audit_filename,O_WRONLY|O_CREAT);
	if(fd<0)
		return fd;
	offset = lseek(fd,0,SEEK_SET);
	ret = write(fd,plc_audit,audit_head_size);
   }

   offset = lseek(fd,0,SEEK_END);

   cmd_template = memdb_get_template(TYPE_PAIR(PLC_OPERATOR,PLC_CMD));

   if(cmd_template == NULL)
	   return -EINVAL;
   ret = struct_2_blob(plc_cmd,DataBuf,cmd_template);
   if(ret<0)
	   return -EINVAL;
   ret = write(fd,DataBuf,ret);
   close(fd);
   ret = plc_audit->event_num;
   Free(plc_audit);
   return 0;
}


int proc_device_audit_store(void * sub_proc,void * recv_msg)
{
    int ret;
    int i=0;
    void * send_msg;
    int fd;
   RECORD(PLC_OPERATOR,PLC_RETURN) * plc_return;
   RECORD(PLC_MONITOR,PLC_AUDIT) * plc_audit;

   void * audit_template;
   void * return_template;
   int audit_head_size;
   char * audit_filename;
   char * dir_name = "device_audit";
   char DataBuf[DIGEST_SIZE*16];

   audit_template = memdb_get_template(TYPE_PAIR(PLC_MONITOR,PLC_AUDIT));
   audit_head_size = struct_size(audit_template);

   ret = message_get_record(recv_msg,&plc_return,0);
   if(ret <0)
	   return ret;
   
   audit_filename = audit_find_last_file(dir_name,plc_return->time);
   if(audit_filename == NULL)
   {
	audit_filename=build_audit_filename(dir_name,plc_return->time);
	if(audit_filename==NULL)
		return -EINVAL;
	plc_audit =Talloc0(audit_head_size);
	if(plc_audit == NULL)
		return -ENOMEM;
	Strncpy(plc_audit->plc_devname,plc_return->plc_devname,DIGEST_SIZE);
	plc_audit->user_type = PLC_OPERATOR;
	plc_audit->start_time = plc_return->time;
	plc_audit->end_time = plc_return->time;
	plc_audit->event_num=1;

	ret = struct_2_blob(plc_audit,DataBuf,audit_template);
	if(ret<0)
		return ret;
	fd = open(audit_filename,O_WRONLY|O_CREAT);
	if(fd<0)
		return fd;
	write(fd,DataBuf,ret);

   }
   else
   {
	fd = open(audit_filename,O_RDONLY);
	if(fd<0)
		return fd;
	plc_audit =Talloc0(audit_head_size);
	if(plc_audit == NULL)
		return -ENOMEM;

	ret = read(fd,plc_audit,audit_head_size);
	if(ret<0)
		return ret;
	close(fd);	
	if(plc_audit->event_num >99)
	{
		audit_filename=build_audit_filename(dir_name,plc_return->time);
		if(audit_filename==NULL)
			return -EINVAL;
		plc_audit =Talloc0(audit_head_size);
		if(plc_audit == NULL)
			return -ENOMEM;
		Strncpy(plc_audit->plc_devname,plc_return->plc_devname,DIGEST_SIZE);
		plc_audit->user_type = PLC_OPERATOR;
		plc_audit->start_time = plc_return->time;
		plc_audit->end_time = plc_return->time;
		plc_audit->event_num=1;
	}
	else
	{

		plc_audit->end_time = plc_return->time;
		plc_audit->event_num++;
	}
	fd = open(audit_filename,O_WRONLY|O_CREAT);
	if(fd<0)
		return fd;
	lseek(fd,SEEK_SET,0);
	write(fd,plc_audit,audit_head_size);
   }

   lseek(fd,SEEK_END,0);

   return_template = memdb_get_template(TYPE_PAIR(PLC_OPERATOR,PLC_RETURN));

   if(return_template == NULL)
	   return -EINVAL;
   ret = struct_2_blob(plc_return,DataBuf,return_template);
   if(ret<0)
	   return -EINVAL;
   write(fd,DataBuf,ret);
   close(fd);
   ret = plc_audit->event_num;
   Free(plc_audit);
   return 0;
}

