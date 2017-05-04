#ifndef __INPUT_MANAGER_H
#define __INPUT_MANAGER_H
#include <sys/time.h>
#include <pthread.h>

/* 输入事件类别 */
#define INPUT_TYPE_STDIN        0
#define INPUT_TYPE_TOUCHSCREEN  1
#define INPUT_TYPE_MOUSE         2

/* 输入事件 */
#define INPUT_VALUE_UP          0   
#define INPUT_VALUE_DOWN        1
#define INPUT_VALUE_EXIT        2
#define INPUT_VALUE_UNKNOWN     -1

typedef struct InputEvent {
	struct timeval tTime; /* 时间 */
	int iType;             /* stdin, touchsceen */
	int iX;                /* 点击X */
	int iY;                /* 点击Y */
	int iKey;              /* 按键值 */
	int iPressure;        /* 压力值 */
}T_InputEvent, *PT_InputEvent;

typedef struct InputOpr
{
	char *name;             /* 输入模块的名字 */
	pthread_t tTreadID;     /* 子线程ID */
	int (*DeviceInit)(void);
	int (*DeviceExit)(void);
	int (*GetInputEvent)(PT_InputEvent ptInputEvent);
	struct InputOpr *ptNext;
} T_InputOpr, *PT_InputOpr;

int InputInit(void);
int RegisterInputOpr(PT_InputOpr ptInputOpr);
void ShowInputOpr(void);
int GetInputEvent(PT_InputEvent ptInputEvent);
int StdInit(void);
int TouchScreenInit(void);
int MouseInit(void);
#endif


