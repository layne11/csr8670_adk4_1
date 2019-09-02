#ifndef _SINK_CUSTOM_H_
#define _SINK_CUSTOM_H_

/*
typedef int(*init_func)(void);
*/

typedef struct _custom_task{
    TaskData task;
    Task clientTask;
    /*init_func custom_init;
    init_func custom_deinit;*/
}custom_task;

int _custom_init(void);
int _custom_deinit(void);
int IntToStr(char *dst, uint32 src, int len);
int StrToInt(uint32 *dst, char *src, int len);
int StrToAddr(char *str, bdaddr *addr);
int AddrToStr(char *str, bdaddr *addr);

#endif
