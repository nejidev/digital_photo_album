#include <input_manager.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>

static PT_InputOpr g_ptInputOprHead;

//全局变量通过互斥体访问
static T_InputEvent g_tInputEvent;

static pthread_mutex_t g_tMutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_tConVar = PTHREAD_COND_INITIALIZER;

int InputInit(void)
{
	int iError = 0;
	
	//iError |= StdInit();
	iError |= TouchScreenInit();
	iError |= MouseInit();
	return iError;
}

int GetInputEvent(PT_InputEvent ptInputEvent)
{
	/* 休眠 */
	pthread_mutex_lock(&g_tMutex);
	pthread_cond_wait(&g_tConVar, &g_tMutex);	

	/* 被唤醒后,返回数据 */
	*ptInputEvent = g_tInputEvent;
	pthread_mutex_unlock(&g_tMutex);
	return 0;
}

static void *InputEventTreadFunction(void *pVoid)
{
	T_InputEvent tInputEvent;
	/* 定义函数指针 */
	int (*GetInputEvent)(PT_InputEvent ptInputEvent);
	GetInputEvent = (int (*)(PT_InputEvent))pVoid;

	while (1)
	{
		//因为有阻塞所以没有输入时是休眠
		if(0 == GetInputEvent(&tInputEvent))
		{
			//有数据时唤醒
			pthread_mutex_lock(&g_tMutex);
			g_tInputEvent = tInputEvent;
			/*  唤醒主线程 */
			pthread_cond_signal(&g_tConVar);
			pthread_mutex_unlock(&g_tMutex);
		}
	}
}

int RegisterInputOpr(PT_InputOpr ptInputOpr)
{
	PT_InputOpr ptTmpInputOpr;
	if(! g_ptInputOprHead)
	{
		g_ptInputOprHead  = ptInputOpr;
		ptInputOpr->ptNext = NULL;
	}
	else
	{
		ptTmpInputOpr = g_ptInputOprHead;
		while(ptTmpInputOpr->ptNext)
		{
			ptTmpInputOpr = ptTmpInputOpr->ptNext;
		}
		ptTmpInputOpr->ptNext = ptInputOpr;
		ptInputOpr->ptNext   = NULL;
	}
	if(0 == ptInputOpr->DeviceInit())
	{
		//初始化成功创建子线程 将子项的GetInputEvent 传进来
		pthread_create(&ptInputOpr->tTreadID, NULL, InputEventTreadFunction, ptInputOpr->GetInputEvent);
	}
	return 0;
}

void ShowInputOpr(void)
{
	int i=0;
	PT_InputOpr ptTmpInputOpr = g_ptInputOprHead;
	while(ptTmpInputOpr)
	{
		printf("display:%d %s \n", i++, ptTmpInputOpr->name);
		ptTmpInputOpr = ptTmpInputOpr->ptNext;
	}
}
