#ifndef LOGIN_USER_H
#define LOGIN_USER_H
 
 
int login_user_init (void * sub_proc, void * para);
int login_user_start (void * sub_proc, void * para);

struct login_init_para
{
	int mode;
}__attribute__((packed));
#endif
