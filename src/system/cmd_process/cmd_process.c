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

#include "plc_emu.h"
#include "plc_dev.h"
#include "modbus_cmd.h"
#include "cmd_process.h"

#define MAX_LINE_LEN 1024


static unsigned char Buf[DIGEST_SIZE*32];

int cmd_process_init(void * sub_proc,void * para)
{
    int ret;

    return 0;
}

int cmd_process_start(void * sub_proc,void * para)
{
    int ret = 0, len = 0, i = 0, j = 0;
    int rc = 0;

    int type,subtype;	
    void * recv_msg;

    void * slot_port;

    // create slot for cmd return 
    
    slot_port = slot_port_init("cmd_hold",1);

    slot_port_addmessagepin(slot_port,TYPE_PAIR(PLC_OPERATOR,PLC_CMD));

    ex_module_addslot(sub_proc,slot_port);

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
			ret=proc_cmd_process(sub_proc,recv_msg);
		}
		else if(type==TYPE(MODBUS_DATA))
		{
			ret=proc_data_process(sub_proc,recv_msg);
		}
	}
    return 0;
}

int proc_cmd_process(void * sub_proc,void * recv_msg)
{
    int ret;
    int i=0;
    void * send_msg;
    time_t curr_time;

   RECORD(PLC_OPERATOR,PLC_CMD) * plc_cmd;
   RECORD(PLC_DEVICE,REGISTER) * plc_reg;
   RECORD(MODBUS_CMD,READ_COILS) * read_switch;
   RECORD(MODBUS_CMD,WRITE_SINGLE_COIL) * switch_cmd;
   RECORD(MODBUS_CMD,READ_HOLDING_REGISTERS) * read_set_cmd;
   RECORD(MODBUS_CMD,READ_INPUT_REGISTERS) * read_env_cmd;
   RECORD(MODBUS_CMD,WRITE_SINGLE_REGISTER) * write_holding_cmd;

   RECORD(MODBUS_DATA,WRITE_SINGLE_COIL) * switch_state;
   RECORD(MODBUS_DATA,READ_HOLDING_REGISTERS) * read_set_state;
   RECORD(MODBUS_DATA,READ_INPUT_REGISTERS) * read_env_state;
   RECORD(MODBUS_DATA,WRITE_SINGLE_REGISTER) * write_holding_state;

   DB_RECORD * db_record;

   void * slot_port;
   void * cube_sock;
   BYTE uuid[DIGEST_SIZE];  

    ret = message_get_record(recv_msg,&plc_cmd,0);
    if(ret<0)
        return ret;
    plc_cmd->time = time(&curr_time);
    // get register from memdb
    
    db_record = memdb_find_first(TYPE_PAIR(PLC_DEVICE,REGISTER),"register_desc",plc_cmd->action_desc);
    if(db_record ==NULL)
    {
           print_cubeerr("cmd_process: no register %s",plc_cmd->action_desc);
           return -EINVAL;
    }
    plc_reg=db_record->record;

    switch(plc_cmd->action)
    {
	  case	ACTION_ON:
	  case ACTION_OFF:
	  	{

            if(plc_reg->type != COILS)
            {
                print_cubeerr("cmd_process: invalid register %s",plc_cmd->action_desc);
                return -EINVAL;
            }

    		switch_cmd=Dalloc0(sizeof(*switch_cmd),NULL);
    		if(switch_cmd == NULL)
        			return -ENOMEM;
			switch_cmd->start_addr=plc_reg->addr;
            if(plc_cmd->action == ACTION_ON)
			    switch_cmd->convert_value=0xff00;
            else if( plc_cmd->action == ACTION_OFF)
			    switch_cmd->convert_value=0x00;

			send_msg = message_create(TYPE_PAIR(MODBUS_CMD,WRITE_SINGLE_COIL),NULL);
			if(send_msg == NULL)
				return -EINVAL;
			message_add_record(send_msg,switch_cmd);

	  	}
	  	break;

	  case ACTION_MONITOR:
	  	{
            		switch(plc_reg->type)
            		{
                		case COILS:
    		        		read_switch=Dalloc0(sizeof(*read_switch),NULL);
    		        		if(read_switch == NULL)
        		        		return -ENOMEM;
	
			        	read_switch->start_addr=plc_reg->addr;
			        	read_switch->reg_num= 1;

			        	send_msg = message_create(TYPE_PAIR(MODBUS_CMD,READ_COILS),recv_msg);
			        	if(send_msg == NULL)
				        	return -EINVAL;
			        	message_add_record(send_msg,read_switch);

                    			break;
                		case DISCRETE_INPUT:

                    			break;
                		case INPUT_REGISTER:
    		        		read_env_cmd=Dalloc0(sizeof(*read_env_cmd),NULL);
    		        		if(read_env_cmd == NULL)
        		        		return -ENOMEM;

			        	read_env_cmd->start_addr=plc_reg->addr;
			        	read_env_cmd->reg_num= 1;

			        	send_msg = message_create(TYPE_PAIR(MODBUS_CMD,READ_INPUT_REGISTERS),recv_msg);
			        	if(send_msg == NULL)
				        	return -EINVAL;
			        	message_add_record(send_msg,read_env_cmd);

                   			break;
               			 case HOLDING_REGISTER:
    		        		read_set_cmd=Dalloc0(sizeof(*read_set_cmd),NULL);
    		        		if(read_set_cmd == NULL)
        		       			return -ENOMEM;

			        	read_set_cmd->start_addr=plc_reg->addr;
			        	read_set_cmd->reg_num= 1;

			        	send_msg = message_create(TYPE_PAIR(MODBUS_CMD,READ_HOLDING_REGISTERS),NULL);
			        	if(send_msg == NULL)
				        	return -EINVAL;
			        	message_add_record(send_msg,read_set_cmd);

                    			break;
                		default:
                    			return -EINVAL; 
            		}

	  	}
	  	break;

	  case ACTION_ADJUST:
	  	{
            		switch(plc_reg->type)
            		{
                		case COILS:
					print_cubeerr("cmd_process: can's adjust coil register");
					return -EINVAL;
                		case DISCRETE_INPUT:
					print_cubeerr("cmd_process: can's adjust discrete register");
					return -EINVAL;
                		case INPUT_REGISTER:
					print_cubeerr("cmd_process: can's adjust input register");
					return -EINVAL;
               			case HOLDING_REGISTER:
    		        		write_holding_cmd=Dalloc0(sizeof(*write_holding_cmd),NULL);
    		        		if(write_holding_cmd == NULL)
        		       			return -ENOMEM;

			        	write_holding_cmd->start_addr=plc_reg->addr;
			        	write_holding_cmd->convert_value = plc_cmd->value;

			        	send_msg = message_create(TYPE_PAIR(MODBUS_CMD,WRITE_SINGLE_REGISTER),NULL);
			        	if(send_msg == NULL)
				        	return -EINVAL;
			        	message_add_record(send_msg,write_holding_cmd);

                    			break;
                		default:
                    			return -EINVAL; 
            		}
	  	}
	  	break;
	default:
		return -EINVAL;
    }	    
	
    // create sock to hold message
    slot_port = ex_module_findport(sub_proc,"cmd_hold");
    if(slot_port == NULL)
        return -EINVAL;
    message_get_uuid(recv_msg,uuid);
    void * sock = slot_create_sock(slot_port,uuid);
    ex_module_addsock(sub_proc,sock);

    // add  send_msg to sock
    ret = slot_sock_addmsg(sock,recv_msg);
    if(ret<=0)
        return -EINVAL;

    // build an uuid expand and add it to send_msg

    RECORD(GENERAL_RETURN,UUID) * uuid_expand;
    uuid_expand = Talloc0(sizeof(*uuid_expand));
    if(uuid_expand == NULL)
        return -ENOMEM;
    uuid_expand->name = dup_str(plc_cmd->plc_devname,0);
    Memcpy(uuid_expand->return_value,uuid,DIGEST_SIZE);
    message_add_expand_data(send_msg,TYPE_PAIR(GENERAL_RETURN,UUID),uuid_expand);
            
    // send modbus command
    ex_module_sendmsg(sub_proc,send_msg);

    // audit plc cmd
    void * audit_msg = message_create(TYPE_PAIR(PLC_OPERATOR,PLC_CMD),NULL);
    if(audit_msg == NULL)
	    return -EINVAL;
    message_add_record(audit_msg,plc_cmd);
    ex_module_sendmsg(sub_proc,audit_msg);

    return 0; 
}

int proc_data_process(void * sub_proc,void * recv_msg)
{
    int ret;
    int i=0;
    void * send_msg;
    int addr;
    int value;
    DB_RECORD * db_record;

   RECORD(PLC_OPERATOR,PLC_RETURN) * plc_data;
   RECORD(PLC_OPERATOR,PLC_CMD) * plc_cmd;
   RECORD(PLC_DEVICE,REGISTER) * plc_reg;
   RECORD(MODBUS_DATA,WRITE_SINGLE_COIL) * switch_state;
   RECORD(MODBUS_DATA,READ_HOLDING_REGISTERS) * read_set_state;
   RECORD(MODBUS_DATA,WRITE_SINGLE_REGISTER) * gear_state;

   void * slot_port;
   void * cube_sock;
    RECORD(GENERAL_RETURN,UUID) * uuid_expand;

   plc_data=Talloc0(sizeof(*plc_data));
   if(plc_data==NULL)
	   return -EINVAL;

   int subtype=message_get_subtype(recv_msg);

   switch(subtype)
   {
	   case SUBTYPE(MODBUS_DATA,WRITE_SINGLE_COIL):
	           ret = message_get_record(recv_msg,&switch_state,0);		   	
		   if(switch_state->convert_value==0xff00)
			plc_data->action = ACTION_ON;
		   else if(switch_state->convert_value ==0x00)
			plc_data->action = ACTION_OFF;
		   plc_data->value = switch_state->convert_value;
		   addr = switch_state->start_addr;	
		   break;
	   case SUBTYPE(MODBUS_DATA,READ_HOLDING_REGISTERS):
	           ret = message_get_record(recv_msg,&read_set_state,0);		   	
		   plc_data->action = ACTION_MONITOR;
		   //addr = read_set_state->start_addr;
		   plc_data->value= *((UINT16 *)read_set_state->value);

		   break;
	   case SUBTYPE(MODBUS_DATA,READ_INPUT_REGISTERS):
		   break;
	   case SUBTYPE(MODBUS_DATA,WRITE_SINGLE_REGISTER):
	           ret = message_get_record(recv_msg,&gear_state,0);		   	
		   plc_data->action = ACTION_ADJUST;
		   addr = gear_state->start_addr;	
		   plc_data->value= gear_state->convert_value;
		   break;
	   default:
		   break;
   } 
  /* 
   db_record = memdb_find_first(TYPE_PAIR(PLC_DEVICE,REGISTER),"addr",&addr);
   if(db_record==NULL)
   	return -EINVAL;
   plc_reg = db_record->record;
*/
 //  plc_data->plc_devname = dup_str(plc_reg->plc_devname,0);
 //  plc_data->action_desc = dup_str(plc_reg->register_desc,0);

  // get uuid from expand
   MSG_EXPAND * msg_expand;
   ret = message_remove_expand(recv_msg,TYPE_PAIR(GENERAL_RETURN,UUID),&msg_expand);
   if(msg_expand == NULL)
	   return -EINVAL;

   void * hold_msg;
   uuid_expand=msg_expand->expand;
   cube_sock = ex_module_removesock(sub_proc,uuid_expand->return_value);
   if(cube_sock == NULL)
        return -EINVAL;
   hold_msg = slot_sock_removemessage(cube_sock,TYPE_PAIR(PLC_OPERATOR,PLC_CMD));
   if(hold_msg == NULL)
        return -EINVAL;
   message_get_record(hold_msg,&plc_cmd,0);		

   plc_data->plc_devname = dup_str(plc_cmd->plc_devname,0);
   plc_data->action_desc = dup_str(plc_cmd->action_desc,0);

   send_msg = message_create(TYPE_PAIR(PLC_OPERATOR,PLC_RETURN),hold_msg);
   if(send_msg == NULL)
	return -EINVAL;
   message_add_record(send_msg,plc_data);
   ex_module_sendmsg(sub_proc,send_msg);
   return 0;
}   
