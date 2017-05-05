#include <page_manager.h>
#include <disp_manager.h>
#include <conf.h>
#include <stdlib.h>
#include <string.h>

PT_PageAction g_ptPageActionHead;

//鼠标图片
static T_Layout g_tMouseLayout = {
	0,0,0,0, "mouse.bmp"
};

//鼠标缩放以后的像素数据
static PT_PixelDatas g_ptMousePixelDatas;

//鼠标遮盖LCD的像素数据
static PT_PixelDatas g_ptMouseMaskPixelDatas;

//鼠标移动后重绘显示 和 主显示 同步 用这个来重绘 LCD
static PT_VideoMem g_ptSyncVideoMem;

//LCD
static PT_DispOpr g_ptDispOpr;

//显示鼠标
int ShowMouse(int x, int y)
{
	unsigned char *videoMem;
	unsigned char *mouseMem;
	int i,j;
	//使用镜像显存重绘LCD
	FlushVideoMemToDev(g_ptSyncVideoMem);
	if((x + g_ptMousePixelDatas->iWidth) <= g_ptDispOpr->iXres && (y + g_ptMousePixelDatas->iHeight) <= g_ptDispOpr->iYres)
	{
		//算法和按钮颜色取反类似
		mouseMem  = g_ptMousePixelDatas->aucPixelDatas;
		videoMem  = g_ptDispOpr->pucDispMem;
		videoMem += y * g_ptDispOpr->iLineWidth;
		videoMem += x * g_ptDispOpr->iBpp / 8;
		for(j = y; j < (y + g_ptMousePixelDatas->iHeight); j++)
		{
			for(i=0; i<g_ptMousePixelDatas->iWidth * g_ptMousePixelDatas->iBpp / 8; i++)
			{
				videoMem[i] = *mouseMem;
				mouseMem++;
			}
			videoMem += g_ptDispOpr->iLineWidth;
		}
		return 0;
	}
	else
	{
		return -1;
	}
}

//初始化鼠标
int InitMouse(void)
{
	//原图
	T_PixelDatas tOriginIconPixelDatas;
	//缩小图
 	T_PixelDatas tIconPixelDatas;
	int iXres, iYres, iBpp;
	int mouse_height;
	int mouse_width;

	//获取一块显存用来保存 重绘 LCD 的像素
	g_ptSyncVideoMem = GetVideoMem(ID("sync"), 1);
	//获取默认dispopr
	g_ptDispOpr = GetDefaultDispDev();
	GetDispResolution(&iXres, &iYres, &iBpp);
	//计算鼠标指针大小 为 高度的 1/20
	mouse_height = iXres / 20;
	mouse_width  = mouse_height;
	g_tMouseLayout.iBottomRightX = mouse_width;
	g_tMouseLayout.iBottomRightY = mouse_height;

	//设置缩放大小
	tOriginIconPixelDatas.iBpp  = iBpp;
	tIconPixelDatas.iBpp        = iBpp;
	tIconPixelDatas.iHeight     = mouse_height;
	tIconPixelDatas.iWidth      = mouse_width;
	tIconPixelDatas.iLineBytes  = tIconPixelDatas.iWidth * tIconPixelDatas.iBpp / 8;
	tIconPixelDatas.iTotalBytes = tIconPixelDatas.iLineBytes * tIconPixelDatas.iHeight;

	//分配缩放后 icon 图片的显存
	tIconPixelDatas.aucPixelDatas = (unsigned char *)malloc(tIconPixelDatas.iTotalBytes);
	if(NULL == tIconPixelDatas.aucPixelDatas)
	{
		DEBUG_PRINTF("malloc tIconPixelDatas error \n");
		return -1;
	}
	//读取 mouse.bmp 的像素
	if(GetPixelDatasForIcon(g_tMouseLayout.strIconName, &tOriginIconPixelDatas))
	{
		free(tIconPixelDatas.aucPixelDatas);
		DEBUG_PRINTF("GetPixelDatasForIcon error \n");
		return -1;
	}
	//缩放到指定大小
	PicZoom(&tOriginIconPixelDatas, &tIconPixelDatas);
	
	//将缩放的小图指针设给全局变量
	g_ptMousePixelDatas = &tIconPixelDatas;

	g_ptMousePixelDatas                = malloc(sizeof(T_PixelDatas) + tIconPixelDatas.iTotalBytes);
	g_ptMousePixelDatas->iBpp          = tIconPixelDatas.iBpp;
	g_ptMousePixelDatas->iHeight       = tIconPixelDatas.iHeight;
	g_ptMousePixelDatas->iWidth        = tIconPixelDatas.iWidth;
	g_ptMousePixelDatas->iLineBytes    = tIconPixelDatas.iLineBytes;
	g_ptMousePixelDatas->iTotalBytes   = tIconPixelDatas.iTotalBytes;
	g_ptMousePixelDatas->aucPixelDatas = (unsigned char *)(g_ptMousePixelDatas + 1);
	memcpy(g_ptMousePixelDatas->aucPixelDatas, tIconPixelDatas.aucPixelDatas, tIconPixelDatas.iTotalBytes);

	//分配一个保存鼠标 遮盖LCD的像素 的显存
	g_ptMouseMaskPixelDatas                = malloc(sizeof(T_PixelDatas) + tIconPixelDatas.iTotalBytes);
	g_ptMouseMaskPixelDatas->iBpp          = tIconPixelDatas.iBpp;
	g_ptMouseMaskPixelDatas->iHeight       = tIconPixelDatas.iHeight;
	g_ptMouseMaskPixelDatas->iWidth        = tIconPixelDatas.iWidth;
	g_ptMouseMaskPixelDatas->iLineBytes    = tIconPixelDatas.iLineBytes;
	g_ptMouseMaskPixelDatas->iTotalBytes   = tIconPixelDatas.iTotalBytes;
	g_ptMouseMaskPixelDatas->aucPixelDatas = (unsigned char *)(g_ptMouseMaskPixelDatas + 1);

	free(tIconPixelDatas.aucPixelDatas);
	free(tOriginIconPixelDatas.aucPixelDatas);
	return 0;
}

//生成 ID 唯一
int ID(char *pcName)
{
	return pcName[0] + pcName[1] + pcName[2] + pcName[3];
}

//生成一页
int GeneratePage(PT_PageLayout ptPageLayout, PT_VideoMem ptVideoMem)
{
	//读取Icon图片缩放到合适大小
	T_PixelDatas tOriginIconPixelDatas;
 	T_PixelDatas tIconPixelDatas;
	PT_Layout atLayout = ptPageLayout->atLayout;
	
	/* 描画数据: VideoMem中的页面数据未生成的情况下才执行下面操作 */
	if (ptVideoMem->ePicState != PS_GENERATED)
	{
		ClearVideoMem(ptVideoMem, COLOR_BACKGROUND);
		tOriginIconPixelDatas.iBpp = ptPageLayout->iBpp;
		tIconPixelDatas.iBpp       = ptPageLayout->iBpp;

		//分配缩放后 icon 图片的显存
		tIconPixelDatas.aucPixelDatas = (unsigned char *)malloc(ptPageLayout->iMaxTotalBytes);
		if(NULL == tIconPixelDatas.aucPixelDatas)
		{
			DEBUG_PRINTF("malloc tIconPixelDatas error \n");
			return -1;
		}
		
		//显示每一个 icon 图片
		while(atLayout->strIconName)
		{
			//得到 BMP 图片数据
			if(GetPixelDatasForIcon(atLayout->strIconName, &tOriginIconPixelDatas))
			{
				free(tIconPixelDatas.aucPixelDatas);
				DEBUG_PRINTF("GetPixelDatasForIcon error \n");
				return -1;
			}
			
			//设置缩放大小
			tIconPixelDatas.iHeight     = atLayout->iBottomRightY - atLayout->iTopLeftY;
			tIconPixelDatas.iWidth      = atLayout->iBottomRightX - atLayout->iTopLeftX;
			tIconPixelDatas.iLineBytes  = tIconPixelDatas.iWidth * tIconPixelDatas.iBpp / 8;
			tIconPixelDatas.iTotalBytes = tIconPixelDatas.iLineBytes * tIconPixelDatas.iHeight;

			//执行缩放
			PicZoom(&tOriginIconPixelDatas, &tIconPixelDatas);
			
			//写入显存
			PicMerge(atLayout->iTopLeftX, atLayout->iTopLeftY, &tIconPixelDatas, &ptVideoMem->tPixelDatas);
			
			//释放 BMP 图片数据内存
			free(tOriginIconPixelDatas.aucPixelDatas);
			atLayout++;
		}
		//释放缩放显存
		free(tIconPixelDatas.aucPixelDatas);

		//设置显存镜像
		memcpy(g_ptSyncVideoMem->tPixelDatas.aucPixelDatas, ptVideoMem->tPixelDatas.aucPixelDatas, ptVideoMem->tPixelDatas.iTotalBytes);
		 
		ptVideoMem->ePicState = PS_GENERATED;
	}
	return 0;
}

//通用的获取事件
int GenericGetInputEvent(PT_PageLayout ptPageLayout, PT_InputEvent ptInputEvent)
{
	T_InputEvent tInputEvent;
	PT_Layout atLayout;
	int i;

	//调用原始的触摸屏数据
	//inputevent 会自动休眠
	GetInputEvent(&tInputEvent);
	*ptInputEvent = tInputEvent;
	
	//绘制鼠标轨迹
	ShowMouse(ptInputEvent->iX, ptInputEvent->iY);
	
	//处理数据，确定是点击哪个按钮上
	i = 0;
	atLayout = ptPageLayout->atLayout;
	while(atLayout[i].strIconName)
	{
		//判断坐标
		if( ptInputEvent->iX >= atLayout[i].iTopLeftX    && 
			ptInputEvent->iX <= atLayout[i].iBottomRightX && 
			ptInputEvent->iY >= atLayout[i].iTopLeftY     &&
			ptInputEvent->iY <= atLayout[i].iBottomRightY)
		{
			//返回按钮的下标
			return i;
		}
		i++;
	}
	return -1;
}

//注册 Page
int RegisterPageAction(PT_PageAction ptPageAction)
{
	PT_PageAction ptTmp;
	if(! g_ptPageActionHead)
	{
		g_ptPageActionHead  = ptPageAction;
	}
	else
	{
		ptTmp = g_ptPageActionHead;
		while(ptTmp->ptNext)
		{
			ptTmp = ptTmp->ptNext;
		}
		ptTmp->ptNext = ptPageAction;
	}
	ptPageAction->ptNext = NULL;
	//如果没有设置输入事件就使用通用的
	if(! ptPageAction->GetInputEvent)
	{
		ptPageAction->GetInputEvent = GenericGetInputEvent;
	}
	return 0;
}

//根据Page Name 查找 Page
PT_PageAction GetPage(char *pcName)
{
 	PT_PageAction ptTmp = g_ptPageActionHead;
	while(ptTmp)
	{
		if(0 == strcmp(ptTmp->name, pcName))
		{
			return ptTmp;
		}
		ptTmp = ptTmp->ptNext;
	}
	return NULL;
}

//总 Page 初始化
int InitPages(void)
{
	int iError;
	iError  = InitMouse();
	iError |= MainPageInit();
	return iError;
}
