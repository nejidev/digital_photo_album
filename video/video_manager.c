#include <video_manager.h>
#include <string.h>

static PT_VideoOpr g_ptVideoOprHead;

int RegisterVideoOpr(PT_VideoOpr ptVideoOpr)
{
	PT_VideoOpr temp;
	if(! g_ptVideoOprHead)
	{
		g_ptVideoOprHead = ptVideoOpr;
	}
	else
	{
		temp = g_ptVideoOprHead;
		while(temp->ptNext)
		{
			temp = temp->ptNext;
		}
		temp->ptNext = ptVideoOpr;
	}
	return 0;
}

PT_VideoOpr GetVideoOpr(char *pcName)
{
	PT_VideoOpr temp = g_ptVideoOprHead;
	while(temp)
	{
		if(0 == strcmp(temp->name, pcName))
		{
			return temp;
		}
		temp = temp->ptNext;
	}
	return NULL;
}

int VideoDeviceInit(char *strDevName, PT_VideoDevice ptVideoDevice)
{
	int iError = 0;
	PT_VideoOpr temp = g_ptVideoOprHead;
	while(temp)
	{
		//初始过每一个设备如果初始化成功就返回
		iError = temp->InitDevice(strDevName, ptVideoDevice);
		if(! iError)
		{
			return 0;
		}
		temp = temp->ptNext;
	}
	return -1;
}

int InitVideoOpr(void)
{
	int iError = 0;
	iError |= V4l2Init();
	return iError;
}

