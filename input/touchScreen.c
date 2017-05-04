#include <input_manager.h>
#include <disp_manager.h>
#include <stdio.h>
#include <stdlib.h>
#include <tslib.h>
#include <conf.h>

static struct tsdev *g_pTSdev;

static int TouchScreenDeviceInit(void);
static int TouchScreenDeviceExit(void);
static int TouchScreenGetInputEvent(PT_InputEvent ptInputEvent);
static T_InputOpr g_tTouchScreeninInputOpr = {
	.name = "TouchScreen",
	.DeviceInit = TouchScreenDeviceInit,
	.DeviceExit = TouchScreenDeviceExit,
	.GetInputEvent = TouchScreenGetInputEvent,
};

static int TouchScreenDeviceInit(void)
{
	char *tsdevice = NULL;
	
	//ts_open(tsdevice,1);
	//多线程时使用阻塞方式打开
	if(NULL != (tsdevice = getenv("TSLIB_TSDEVICE"))) 
	{
         g_pTSdev = ts_open(tsdevice, 0);
    } 
	else 
	{
	     g_pTSdev = ts_open("/dev/input/input0", 0);
    }

	if (! g_pTSdev) 
	{
		DEBUG_PRINTF("ts_open error\n");
		return -1;
	}

	if (ts_config(g_pTSdev)) 
	{
		DEBUG_PRINTF("ts_open error\n");
		return -1;
	}
	return 0;
}

static int TouchScreenDeviceExit(void)
{
	return 0;
}

static int TouchScreenGetInputEvent(PT_InputEvent ptInputEvent)
{
	struct ts_sample samp;
	int ret;
	static struct timeval tPreTime;
	
	ret = ts_read(g_pTSdev, &samp, 1);

	if (ret < 0) 
	{
		return -1;
	}

	if (ret != 1)
	{
		return -1;
	}
	//上报事件
	ptInputEvent->iType     = INPUT_TYPE_TOUCHSCREEN;
	ptInputEvent->tTime     = samp.tv;
	ptInputEvent->iX        = samp.x;
	ptInputEvent->iY        = samp.y;
	ptInputEvent->iPressure = samp.pressure;
	return 0;
}

int TouchScreenInit(void)
{
	return RegisterInputOpr(&g_tTouchScreeninInputOpr);
}
