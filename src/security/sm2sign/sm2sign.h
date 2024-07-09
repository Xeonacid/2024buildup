#ifndef SM2_SIGN_FUNC_H
#define SM2_SIGN_FUNC_H

struct init_struct
{
	int  mode;   // 0 means signer, 1 means verifier;
	char * prikeyfile;
	char * pubkeyfile;
};

// plugin's init func and kickstart func
int sm2sign_init(void * sub_proc,void * para);
int sm2sign_start(void * sub_proc,void * para);

#endif
