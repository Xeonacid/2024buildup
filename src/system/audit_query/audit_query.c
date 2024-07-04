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
#include "audit_query.h"

#define MAX_LINE_LEN 1024


static unsigned char Buf[DIGEST_SIZE*32];

static NAME2VALUE dir_list[] = {
    {"engineer_audit",PLC_ENGINEER},
    {"operator_audit",PLC_OPERATOR},
    {"monitor_audit",PLC_MONITOR},
    {"device_audit",PLC_DEVICE},
    {NULL,0}
};

int audit_query_init(void * sub_proc,void * para)
{
    int ret;

    return 0;
}

int audit_query_start(void * sub_proc,void * para)
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
			// store operator command audit	
			ret=proc_audit_query(sub_proc,recv_msg);
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

char * _get_auditdir(int type)
{
    int i;
    for(i=0;dir_list[i].name!=NULL;i++)
    {
        if(dir_list[i].value == type)
            return dir_list[i].name;
    }
    return NULL;
}

int proc_audit_query(void * sub_proc,void * recv_msg)
{
    int ret;
    int i=0;
    void * send_msg;
    int fd;
   RECORD(PLC_MONITOR,PLC_CMD) * plc_cmd;

   
   ret = message_get_record(recv_msg,&plc_cmd,0);
   if(ret <0)
	   return ret;

   if(plc_cmd->action != ACTION_AUDIT)
        return -EINVAL;

   if(plc_cmd->action_desc == NULL)
       return -EINVAL;

   if(Strcmp(plc_cmd->action_desc,"operator")==0)
   {
        return proc_operator_audit_query(sub_proc,recv_msg,plc_cmd);     
   }
   return -EINVAL;
}

int proc_operator_audit_query(void * sub_proc,void * recv_msg,
    RECORD(PLC_MONITOR,PLC_CMD) * plc_cmd)
{

   int ret;
   void * audit_template;
   void * cmd_template;
   int audit_head_size;
   char * audit_filename;
   char * dir_name ;
   char DataBuf[DIGEST_SIZE*16];

   int i,j;
   int fd;
   int offset;
   int block_size=DIGEST_SIZE*16; 
   int type;
   int subtype;
   void * audit_data;

   void * send_msg;

   RECORD(PLC_MONITOR,PLC_AUDIT) * plc_audit;

   int data_offset,left_size;
   int count;


   audit_template = memdb_get_template(TYPE_PAIR(PLC_MONITOR,PLC_AUDIT));
   audit_head_size = struct_size(audit_template);

    
   if(Strcmp(plc_cmd->action_desc,"operator") == 0)
   {
            type= TYPE(PLC_OPERATOR);
            subtype =SUBTYPE(PLC_OPERATOR,PLC_CMD);
            cmd_template = memdb_get_template(type,subtype);
            dir_name = _get_auditdir(PLC_OPERATOR);

            if(dir_name ==NULL)
                return -EINVAL;
   }
   else if(Strcmp(plc_cmd->action_desc,"device") == 0)
   {
            type= TYPE(PLC_OPERATOR);
            subtype =SUBTYPE(PLC_OPERATOR,PLC_RETURN);
            cmd_template = memdb_get_template(type,subtype);
            dir_name = _get_auditdir(PLC_DEVICE);

            if(dir_name ==NULL)
                return -EINVAL;
   }
  

  // in audit action, value1 is time, value2 is no;
   audit_filename = audit_find_next_file(dir_name,plc_cmd->value1);
  
   if(audit_filename == NULL)
        return -EINVAL;

	plc_audit =Talloc0(audit_head_size);
	if(plc_audit == NULL)
		return -ENOMEM;

    count =0;
	fd = open(audit_filename,O_RDONLY);
	if(fd<0)
		return fd;

    if((plc_cmd->value2 <=0)||(plc_cmd->value2>10))
        return -EINVAL;

	ret = read(fd,DataBuf,audit_head_size);
    if(ret<0)
        return -EINVAL;

    ret = blob_2_struct(DataBuf,plc_audit,audit_template);
    if(ret<0)
    {
          print_cubeerr("audit_query: read head data from %s error",audit_filename);
          return -EINVAL;
     }


    audit_data = Talloc0(struct_size(cmd_template));
    if(audit_data==NULL)
         return -ENOMEM;

    send_msg=NULL;
    int data_left=1;
    while(count < plc_cmd->value2)
    {

        left_size=0;
        offset=0;
        

        for(i=0;i<plc_audit->event_num;i++)
        {
            if((data_left) && (left_size < DIGEST_SIZE*8))
            {
                Memcpy(DataBuf,DataBuf+offset,left_size);
                ret =read(fd,DataBuf+left_size,block_size-left_size);
                if(ret <0)
                     return ret;
                if(ret< block_size-left_size)
                    data_left=0;
                left_size+=ret;
            }
            ret = blob_2_struct(DataBuf+offset,audit_data,cmd_template);
            if(ret<0)
            {
                print_cubeerr("audit_query: read data %d from %s error",i,audit_filename);
                return -EINVAL;
            }
            left_size-=ret;
            offset+=ret;
            int comp_time;
            ret = struct_read_elem("time",audit_data,&comp_time,cmd_template);
            if(ret <0)
                return -EINVAL;
            if(comp_time<plc_cmd->value1)
                continue;
            if(send_msg == NULL)
            {
                send_msg = message_create(type,subtype,recv_msg);
                if(send_msg == NULL)
                    return -EINVAL;
            }
            message_add_record(send_msg,audit_data);
            count++;
        }    
        close(fd);
    }

    if(send_msg != NULL)
        ex_module_sendmsg(sub_proc,send_msg);
    else
        ex_module_sendmsg(sub_proc,recv_msg);
   return 0;
}
