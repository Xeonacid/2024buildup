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

#include "plc_emu.h"
#include "sm2sign.h"


static int mode =0;
BYTE prikey[DIGEST_SIZE*2];
BYTE pubkey_XY[64];
unsigned long prilen=DIGEST_SIZE*2;
char * prikeyfile=NULL;
char * pubkeyfile=NULL;
BYTE Buf[DIGEST_SIZE*16];

char * sign_file_path = "/root/2024buildup/src/logic/thermostat_logic/";
char * verify_file_path = "/root/2024buildup/plugin/";

int sm2sign_init(void * sub_proc,void * para)
{
	int ret;
	int fd;
	// add youself's plugin init func here
    	struct sm2_init_struct * init_para=para;
	char Buf[128];
    	if(para==NULL)	 
		return -EINVAL;

	mode=init_para->mode;

	if(init_para->prikeyfile!=NULL)
		prikeyfile=dup_str(init_para->prikeyfile,0);

	if(init_para->pubkeyfile!=NULL)
		pubkeyfile=dup_str(init_para->pubkeyfile,0);

	if(mode==0) // 签名者初始化过程
	{
		if(prikeyfile == NULL)
			return -EINVAL;
		if(access(prikeyfile,R_OK)!=0)
		{
			// 可能需要创建密钥
			if(access(prikeyfile,F_OK)==0)
			{
				print_cubeerr("gm2_sign: can't read keyfile");
				return -EINVAL;
			}
	        	ret=GM_GenSM2keypair(prikey,&prilen,pubkey_XY);
        		if(ret!=0)
                		return -EINVAL;
			fd = open(prikeyfile,O_WRONLY|O_CREAT|O_TRUNC,00400);
			if(fd<0)
			{
				print_cubeerr("gm2_sign: can't open prikeyfile");
				return fd;
			}
			write(fd,prikey,prilen);	
			close(fd);

			fd = open(pubkeyfile,O_WRONLY|O_CREAT|O_TRUNC,00444);
			if(fd<0)
			{
				print_cubeerr("gm2_sign: can't open pubkeyfile");
				return fd;
			}
			write(fd,pubkey_XY,64);
			close(fd);

		}
		else
		{
			//尝试从文件中读出公私钥对
			fd = open(prikeyfile,O_RDONLY);
			if(fd<0)
			{
				print_cubeerr("gm2_sign: can't read prikeyfile");
				return fd;
			}
			ret=read(fd,prikey,DIGEST_SIZE*2);
			if(ret<0)
			{
				print_cubeerr("gm2_sign: prikey read abnormal!");
				return -EINVAL;
			}
			prilen=ret;
			close(fd);

			fd = open(pubkeyfile,O_RDONLY);
			if(fd<0)
			{
				print_cubeerr("gm2_sign: can't open pubkeyfile");
				return fd;
			}
			read(fd,pubkey_XY,64);
			close(fd);
		}
		// 签名者初始化过程结束
	}
	else if(mode ==1)  //验签者初始化过程
	{
		printf("enter sm2 verify init!\n");
		if(pubkeyfile == NULL)
			return -EINVAL;
		fd = open(pubkeyfile,O_RDONLY);
		if(fd<0)
		{
			print_cubeerr("gm2_sign: can't open pubkeyfile");
			return fd;
		}
		read(fd,pubkey_XY,64);
		close(fd);
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
		if((type==TYPE(PLC_ENGINEER)) && (subtype==SUBTYPE(PLC_ENGINEER,LOGIC_UPLOAD)))
		{
			printf("enter sm2 verify process mode %d!\n",mode);
			if(mode==0)
			{
				proc_sm2_sign(sub_proc,recv_msg);
			}
			else if(mode==1)
			{
				proc_sm2_verify(sub_proc,recv_msg);
			}
		}
	}
	return 0;
}

int proc_sm2_sign(void * sub_proc,void * recv_msg)
{
        int i;
        int ret=0;

	BYTE DataBuf[DIGEST_SIZE*8];
	BYTE SignBuf[DIGEST_SIZE*4];
	int signlen;
	BYTE UserID[DIGEST_SIZE];
	unsigned long lenUID=DIGEST_SIZE;
	Memset(UserID,'A',DIGEST_SIZE);

	BYTE file_digest[DIGEST_SIZE];
	BYTE cmd_digest[DIGEST_SIZE];
	RECORD(PLC_ENGINEER,LOGIC_UPLOAD) * bin_upload;
	RECORD(GENERAL_RETURN,BINDATA) * sign_data;

	ret=message_get_record(recv_msg,&bin_upload,0);
	if(ret<0)
		return ret;

	// 计算文件的摘要值并与bin_upload中的uuid对比
	
	// 使用calculate_sm3函数

	//序列化命令
	
	void * cmd_template = memdb_get_template(TYPE_PAIR(PLC_ENGINEER,LOGIC_UPLOAD));
	if(cmd_template == NULL)
		return -EINVAL;
	ret = struct_2_blob(bin_upload,DataBuf,cmd_template);
	if(ret<0)
		return ret;

	//对命令进行签名
	GM_SM2Sign(SignBuf, &signlen, DataBuf, ret, UserID, lenUID, prikey, prilen);

	sign_data = Talloc0(sizeof(*sign_data));
	if(sign_data == NULL)
		return -ENOMEM;
	sign_data->name = dup_str("signed data", 0);
	sign_data->size = signlen;
	sign_data->bindata = Talloc0(sign_data->size);
	Memcpy(sign_data->bindata, SignBuf, signlen);
	
	// 签名结束，签名内容输出到sign_data中

	message_add_expand_data(recv_msg,TYPE_PAIR(GENERAL_RETURN,BINDATA),sign_data);

	ex_module_sendmsg(sub_proc,recv_msg);

        return ret;
}

int proc_sm2_verify(void * sub_proc,void * recv_msg)
{
        int i;
        int ret=0;

	BYTE DataBuf[DIGEST_SIZE*8];
	BYTE VerifyBuf[DIGEST_SIZE*2];
	int signlen;
	BYTE UserID[DIGEST_SIZE];
	unsigned long lenUID=DIGEST_SIZE;
	Memset(UserID,'A',DIGEST_SIZE);

	BYTE file_digest[DIGEST_SIZE];
	BYTE cmd_digest[DIGEST_SIZE];
	RECORD(PLC_ENGINEER,LOGIC_UPLOAD) * bin_upload;
	RECORD(GENERAL_RETURN,BINDATA) * sign_data;
	RECORD(GENERAL_RETURN,INT) * verify_result;
	MSG_EXPAND * msg_expand;

	ret=message_get_record(recv_msg,&bin_upload,0);
	if(ret<0)
		return ret;
	int result;

	ret=message_remove_expand(recv_msg,TYPE_PAIR(GENERAL_RETURN,BINDATA),&msg_expand);
	if(msg_expand == NULL)
		return -EINVAL;

	sign_data = msg_expand->expand;

	//序列化命令
	
	void * cmd_template = memdb_get_template(TYPE_PAIR(PLC_ENGINEER,LOGIC_UPLOAD));
	if(cmd_template == NULL)
		return -EINVAL;
	ret = struct_2_blob(bin_upload,DataBuf,cmd_template);
	if(ret<0)
		return ret;

	//对命令进行验证
	verify_result = Talloc0(sizeof(*verify_result));
	if(verify_result == NULL)
		return -ENOMEM;
	verify_result->name = dup_str("verify result", 0);
	verify_result->return_value = GM_SM2VerifySig(sign_data->bindata, sign_data->size, DataBuf, ret, UserID, lenUID, pubkey_XY, 64);
	
	// 计算文件的摘要值并与bin_upload中的uuid对比
	
	// 使用calculate_sm3函数 
	// 如验证结果不一致，则verify_result->result = 1
	
	//验证结束，验证内容输出到verify_result中

	message_add_expand_data(recv_msg,TYPE_PAIR(GENERAL_RETURN,INT),verify_result);

	ex_module_sendmsg(sub_proc,recv_msg);

        return ret;
}
