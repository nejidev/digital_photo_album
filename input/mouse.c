#include <input_manager.h>
#include <disp_manager.h>
#include <conf.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <linux/input.h>

#define MOUSE_DEV "/dev/input/mouse0"

static int postion_x;
static int postion_y;
//限定最大 X  Y 	
int postion_max_x;	
int postion_max_y;
//移动间距
int mouse_move_step;
static PT_DispOpr ptDispOpr;
static int mouse_fd;


static int MouseDeviceInit(void);
static int MouseDeviceExit(void);
static int MouseGetInputEvent(PT_InputEvent ptInputEvent);
static T_InputOpr g_tMouseinInputOpr = {
	.name          = "mouse",
	.DeviceInit    = MouseDeviceInit,
	.DeviceExit    = MouseDeviceExit,
	.GetInputEvent = MouseGetInputEvent,
};

static int MouseDeviceInit(void)
{
	ptDispOpr = GetDefaultDispDev();
	if(! ptDispOpr)
	{
		DEBUG_PRINTF("mouse cat't get lcd params \n");
		return -1;
	}
	//打开鼠标设备
	mouse_fd = open(MOUSE_DEV, O_RDONLY);
	if(-1 == mouse_fd)
	{
		DEBUG_PRINTF("mouse cat't open %s \n", MOUSE_DEV);
		return -1;
	}
	//最大的X Y 为 LCD 的高度 宽度
	postion_max_x = ptDispOpr->iXres;
	postion_max_y = ptDispOpr->iYres;
	//默认出现在LCD正中间
	postion_x      = postion_max_x / 2;
	postion_y      = postion_y / 2;
	//鼠标每次移动屏幕的1/10 长度
	mouse_move_step = postion_max_x / 50;
	return 0;
}

static int MouseDeviceExit(void)
{
	close(mouse_fd);
	return 0;
}

static int MouseGetInputEvent(PT_InputEvent ptInputEvent)
{
    //处理鼠标事件 目前只处理 单志 移动
    unsigned char buf[3];
	ptInputEvent->iType = INPUT_TYPE_MOUSE;
	gettimeofday(&ptInputEvent->tTime, NULL); 
	if(read(mouse_fd, buf, sizeof(buf)))
	{
		/**
		 * 原理 当不为0 时说明鼠标在移动，经测试发现，值为 12 或 255 254 所以这里取比10小就是减少
		 */
		//X 向右移动时变大 没问题
		if(0 < buf[1])
		{
			postion_x += (10 > buf[1]) ? mouse_move_step : (0 - mouse_move_step);
		}
		//Y 向下移动时变小 需要反转
		if(0 < buf[2])
		{
			postion_y += (10 > buf[2]) ? (0 - mouse_move_step) : mouse_move_step;
		}
		postion_x = (1 > postion_x) ? 0 : postion_x;
		postion_y = (1 > postion_y) ? 0 : postion_y;
		postion_x = (postion_max_x < postion_x) ? postion_max_x : postion_x;
		postion_y = (postion_max_y < postion_y) ? postion_max_y : postion_y;
			
		ptInputEvent->iX = postion_x;
		ptInputEvent->iY = postion_y;
		//按下左键
		if(0x9 == buf[0])
		{
			ptInputEvent->iPressure = 1;
		}
		//松开按键
		if(0x8 == buf[0])
		{
			ptInputEvent->iPressure = 0;
		}
		return 0;
	}
	return -1;
}

int MouseInit(void)
{
	return RegisterInputOpr(&g_tMouseinInputOpr);
}


