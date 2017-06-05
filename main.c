#include <stdio.h>
#include <draw.h>
#include <disp_manager.h>
#include <page_manager.h>
#include <fonts_manager.h>
#include <file.h>

/*
测试方法
需要有ts.ko
insmod ts.ko
export TSLIB_TSDEVICE=/dev/input/event0
export TSLIB_CALIBFILE=/etc/pointercal
export TSLIB_CONFFILE=/etc/ts.conf
export TSLIB_PLUGINDIR=/lib/ts
export TSLIB_CONSOLEDEVICE=none
export TSLIB_FBDEVICE=/dev/fb0

第一次使用需要校准
ts_calibrate
*/
int main(int argc, char **argv)
{
	PT_DispOpr ptDispOpr;
	T_PageParams tPageParams = { .intvalSceond = 1 , .strSelectDir = "/" };
	PT_FontOpr ptFontOpr;

	//初始化LCD
	DisplayInit();
	SelectDefaultDispDev("fb");
	
	ptDispOpr = GetDefaultDispDev();
	ptDispOpr->CleanScreen(0);

	//初始化freetype  
	InitFonts();
	ptFontOpr = GetFontOpr("freetype");
	ptFontOpr->FontInit("msyh.ttf", 12);

	//初始化字符解码
	EncodingInit();
	
	//分配5个显存
	AllocVideoMem(5);

	//初始化事件
	InputInit();

	//初始化 图片解析
	InitParse();
	
	//初始化页面
	InitPages();

	//运行第一个页面
	GetPage("main")->Run(&tPageParams);
	
	return 0;
}

