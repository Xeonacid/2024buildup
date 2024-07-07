#ifndef FILE_REPLACE_FUNC_H
#define FILE_REPLACE_FUNC_H


// plugin's init func and kickstart func
int file_replace_init(void * sub_proc,void * para);
int file_replace_start(void * sub_proc,void * para);
struct timeval time_val={0,50*1000};

#endif
