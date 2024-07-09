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
#include "file_struct.h"
#include "sm2.h"

#include "sm2sign.h"


int mode =0;
BYTE prikey[DIGEST_SIZE*2];
BYTE pubkey_XY[64];
unsigned long prilen=DIGEST_SIZE*2;
char * prikeyfile=NULL;
char * pubkeyfile=NULL;
BYTE Buf[DIGEST_SIZE*16];

int sm2sign_init(void * sub_proc,void * para)
{
	int ret;
	// add youself's plugin init func here
    	struct init_struct * init_para=para;
    	if(para==NULL)	 
		return -EINVAL;

	mode=init_para->mode;

	if(mode==0)
	{

        	ret=GM_GenSM2keypair(prikey,&prilen,pubkey_XY);
        	if(ret!=0)
                	return -EINVAL;
	}
	return 0;
}

int sm2sign_start(void * sub_proc,void * para)
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
		if(type==NULL)
		{
			message_free(recv_msg);
			continue;
		}
		if(!memdb_find_recordtype(type,subtype))
		{
			print_cubeerr("message format (%d %d) is not registered!\n",
				message_get_type(recv_msg),message_get_subtype(recv_msg));
			ex_module_sendmsg(sub_proc,recv_msg);
			continue;
		}
		if((type==TYPE(MESSAGE)) && (subtype==SUBTYPE(MESSAGE,BASE_MSG)))
		{
			if(mode==0)
				proc_pubkey_send(sub_proc,recv_msg);
			else if(mode==1)
				proc_pubkey_recover(sub_proc,recv_msg);
		}
		else if((type==TYPE(FILE_TRANS)) && (subtype==SUBTYPE(FILE_TRANS,FILE_DATA)))
		{
			if(mode==1)
			{
				proc_sessionkey_gen(sub_proc,recv_msg);
			}
			else if(mode==0)
			{
				proc_sessionkey_recover(sub_proc,recv_msg);
			}
		}
	}
	return 0;
}

int proc_pubkey_send(void * sub_proc,void * message)
{
        int i;
        int ret=0;
	RECORD(MESSAGE,BASE_MSG) * base_msg;

	RECORD(GENERAL_RETURN,BINDATA) * pubkeydata;

	ret=message_get_record(message,&base_msg,0);
	if(ret<0)
		return ret;
	else if (base_msg==NULL)
		return -EINVAL;

	if(Strncmp(base_msg->message,"sendkey",DIGEST_SIZE)!=0)
		return 0;

	pubkeydata = Talloc0(sizeof(*pubkeydata));
	if(pubkeydata==NULL)
		return -ENOMEM;
	pubkeydata->name=dup_str("pubkeydata",0);
	pubkeydata->size=64;
	pubkeydata->bindata=Talloc0(64);
	Memcpy(pubkeydata->bindata,pubkey_XY,64);
		
	message_add_expand_data(message,TYPE_PAIR(GENERAL_RETURN,BINDATA),pubkeydata); 

        ret=ex_module_sendmsg(sub_proc,message);
        return ret;
}

int proc_pubkey_recover(void * sub_proc,void * message)
{
	int ret=1;
	int i;
	RECORD(MESSAGE,BASE_MSG) * base_msg;
	MSG_HEAD* msghead;
	MSG_EXPAND * msg_expand;
	RECORD(GENERAL_RETURN,BINDATA) * pubkeydata=NULL;

	ret=message_get_record(message,&base_msg,0);
	if(ret<0)
		return ret;
	else if (base_msg==NULL)
		return -EINVAL;

	if(Strncmp(base_msg->message,"sendkey",DIGEST_SIZE)!=0)
		return 0;

	pubkeydata = Talloc0(sizeof(*pubkeydata));
	if(pubkeydata==NULL)
		return -ENOMEM;
	msghead= message_get_head(message);
	if(msghead==NULL)
		return -EINVAL;
	
	for(i=0;i<msghead->expand_num;i++)
	{
		ret=message_get_expand(message,&msg_expand,i);
		if(msg_expand==NULL)
			return -EINVAL;
		if((msg_expand->type==TYPE(GENERAL_RETURN)) &&
			(msg_expand->subtype==SUBTYPE(GENERAL_RETURN,BINDATA)))
		{
			pubkeydata=msg_expand->expand;
			if(pubkeydata==NULL)
				continue;
			if(Strcmp(pubkeydata->name,"pubkeydata"))
			{
				pubkeydata=NULL;
				continue;
			}		
			else
			{
				ret= message_remove_indexed_expand(message,i,&msg_expand);
				pubkeydata=msg_expand->expand;
				Memcpy(pubkey_XY,pubkeydata->bindata,pubkeydata->size);
				break;
			}
		}
	}		
	return ret;
}

int proc_sessionkey_gen(void * sub_proc,void * message)
{
        int i;
        int ret;

	RECORD(GENERAL_RETURN,BINDATA) * keyblob;
	RECORD(GENERAL_RETURN,UUID) * sessionkey;

	keyblob = Talloc0(sizeof(*keyblob));
	if(keyblob==NULL)
		return -ENOMEM;

	sessionkey = Talloc0(sizeof(*sessionkey));
	if(keyblob==NULL)
		return -ENOMEM;

	ret=RAND_bytes(sessionkey->return_value,DIGEST_SIZE);

	int Enclen=DIGEST_SIZE*16;
	ret=GM_SM2Encrypt(Buf,&Enclen,sessionkey->return_value,DIGEST_SIZE,pubkey_XY,64);
	if(ret!=0)
	{
		print_cubeerr("Sm2 Encrypt sessionkey failed!");
		return -EINVAL;
	}
	
	keyblob->name=dup_str("sessionkeyblob",0);
	keyblob->size=Enclen;
	keyblob->bindata=Talloc0(Enclen);
	Memcpy(keyblob->bindata,Buf,Enclen);
		
	message_add_expand_data(message,TYPE_PAIR(GENERAL_RETURN,BINDATA),keyblob); 

	sessionkey->name=dup_str("sessionkey",0);

	message_add_expand_data(message,TYPE_PAIR(GENERAL_RETURN,UUID),sessionkey); 

        ex_module_sendmsg(sub_proc,message);
        return ret;
}

int proc_sessionkey_recover(void * sub_proc,void * message)
{
        int i;
        int ret;

	MSG_HEAD* msghead;
	MSG_EXPAND * msg_expand;
	RECORD(GENERAL_RETURN,BINDATA) * keyblob=NULL;
	RECORD(GENERAL_RETURN,UUID) * sessionkey=NULL;

	msghead= message_get_head(message);
	if(msghead==NULL)
		return -EINVAL;
	
	for(i=0;i<msghead->expand_num;i++)
	{
		ret=message_get_expand(message,&msg_expand,i);
		if(msg_expand==NULL)
			return -EINVAL;
		if((msg_expand->type==TYPE(GENERAL_RETURN)) &&
			(msg_expand->subtype==SUBTYPE(GENERAL_RETURN,BINDATA)))
		{
			keyblob=msg_expand->expand;
			if(keyblob==NULL)
				continue;
			if(Strcmp(keyblob->name,"sessionkeyblob"))
			{
				keyblob=NULL;
				continue;
			}		
			else
			{
				// recover the sessionkey
				sessionkey=Talloc0(sizeof(*sessionkey));
				if(sessionkey==NULL)
					return -ENOMEM;
				ret= message_remove_indexed_expand(message,i,&msg_expand);
				keyblob=msg_expand->expand;
				
				int Enclen=DIGEST_SIZE*16;
				ret=GM_SM2Decrypt(Buf,&Enclen,keyblob->bindata,keyblob->size,prikey,prilen);
				if(ret!=0)
				{
					print_cubeerr("decrypt session key failed!\n");
					return -EINVAL;
				}
				if(Enclen!=DIGEST_SIZE)
				{
					print_cubeerr("session key size error!\n");
					return -EINVAL;
				}

				Memcpy(sessionkey->return_value,Buf,Enclen);
				sessionkey->name=dup_str("sessionkey",0);
				message_add_expand_data(message,TYPE_PAIR(GENERAL_RETURN,UUID),sessionkey); 
	
				break;
			}
		}
	}
	
        ex_module_sendmsg(sub_proc,message);
        return ret;
}
