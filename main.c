#include <stdio.h>
#include <draw.h>
#include <disp_manager.h>
#include <page_manager.h>
#include <fonts_manager.h>
#include <encoding_manager.h>
#include <convert_manager.h>
#include <video_manager.h>
#include <file.h>

/*
���Է���
��Ҫ��ts.ko
insmod ts.ko
export TSLIB_TSDEVICE=/dev/input/event0
export TSLIB_CALIBFILE=/etc/pointercal
export TSLIB_CONFFILE=/etc/ts.conf
export TSLIB_PLUGINDIR=/lib/ts
export TSLIB_CONSOLEDEVICE=none
export TSLIB_FBDEVICE=/dev/fb0

��һ��ʹ����ҪУ׼
ts_calibrate
*/
int main(int argc, char **argv)
{
	PT_DispOpr ptDispOpr;
	T_PageParams tPageParams = { .intvalSceond = 1 , .strSelectDir = "/" };
	PT_FontOpr ptFontOpr;

	//��ʼ��LCD
	DisplayInit();
	SelectDefaultDispDev("fb");
	
	ptDispOpr = GetDefaultDispDev();
	ptDispOpr->CleanScreen(0);

	//��ʼ��freetype  
	InitFonts();
	ptFontOpr = GetFontOpr("freetype");
	ptFontOpr->FontInit("msyh.ttf", 12);

	//��ʼ���ַ�����
	EncodingInit();
	
	//����5���Դ�
	AllocVideoMem(5);

	//��ʼ���¼�
	InputInit();

	//��ʼ�� ͼƬ����
	InitParse();

	//��ʼ������ͷ
	InitVideoOpr();
	
	//��ʼ����Ƶת����
	InitVideoConvert();
	
	//��ʼ��ҳ��
	InitPages();

	//���е�һ��ҳ��
	GetPage("main")->Run(&tPageParams);
	
	return 0;
}

