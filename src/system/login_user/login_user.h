#ifndef LOGIN_USER_H
#define LOGIN_USER_H
 
 
int login_user_init (void * sub_proc, void * para);
int login_user_start (void * sub_proc, void * para);

struct login_user_init_para
{
    int mode; // 0 - plain passwd 
              // 1 - sm3 hash passwd 
              // 2 - challenge-response login    
}__attribute__((packed));

#endif
