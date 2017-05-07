#include <conf.h>
#include <render.h>
#include <page_manager.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int g_iInteval = 1;
static int GenerateInterval(PT_VideoMem ptVideoMem);
static int GenerateInterval(PT_VideoMem ptVideoMem);

//icon图标
static T_Layout g_atIntevalPageIconsLayout[] = {
	{0, 0, 0, 0, "time_inc.bmp"},
	{0, 0, 0, 0, "time.bmp"},
	{0, 0, 0, 0, "time_add.bmp"},
	{0, 0, 0, 0, "ok.bmp"},
	{0, 0, 0, 0, "cancel.bmp"},
	{0, 0, 0, 0, NULL},
};

//Inteval Page layout
static T_PageLayout g_tIntevalPageLayout = {
	.iMaxTotalBytes = 0,
	.atLayout       = g_atIntevalPageIconsLayout,
};

static int ReGenerateInterval()
{
	PT_VideoMem ptVideoMem = GetVideoMem(ID("sync"), 1);
	return GenerateInterval(ptVideoMem);
}

static int GenerateInterval(PT_VideoMem ptVideoMem)
{
	int iXres;
	int iYres;
	int iBpp;
	char strNumber[3];
	T_PixelDatas tPixelDatas;
	snprintf(strNumber, 3, "%02d", g_iInteval);

	//计算要显示的位置及大小
	tPixelDatas.iWidth  = g_atIntevalPageIconsLayout[1].iBottomRightX - g_atIntevalPageIconsLayout[1].iTopLeftX;
	tPixelDatas.iHeight = g_atIntevalPageIconsLayout[1].iBottomRightY - g_atIntevalPageIconsLayout[1].iTopLeftY;
	tPixelDatas.iWidth  = tPixelDatas.iWidth  /2;
	tPixelDatas.iHeight = tPixelDatas.iHeight /2;
	
	//测量LCD的信息
	GetDispResolution(&iXres, &iYres, &iBpp);
	tPixelDatas.iBpp = iBpp;
	tPixelDatas.iLineBytes = tPixelDatas.iWidth * tPixelDatas.iBpp / 8;
	tPixelDatas.iTotalBytes = tPixelDatas.iLineBytes * tPixelDatas.iHeight;
	//分配显示字符用的内存
	tPixelDatas.aucPixelDatas = (unsigned char *)malloc(tPixelDatas.iTotalBytes);
	if(! tPixelDatas.aucPixelDatas)
	{
		DEBUG_PRINTF("GenerateInterval malloc error \n");
		return -1;
	}

	//清为背景色
	ClearPixelDatasMem(&tPixelDatas, COLOR_BACKGROUND);
	
	//使用 freetype 或 汉字库 或 ascii 生成数据
	if(GetPixelDatasForFreetype(strNumber, &tPixelDatas))
	{
		DEBUG_PRINTF("GetPixelDatasForFreetype error \n");
		return -1;
	}
	
	//写入 videoMem
	if(MergePixelDatasToVideoMem(tPixelDatas.iHeight + g_atIntevalPageIconsLayout[1].iTopLeftX, 
									tPixelDatas.iHeight /2 + g_atIntevalPageIconsLayout[1].iTopLeftY, 
									&tPixelDatas, ptVideoMem))
	{
		DEBUG_PRINTF("MergePixelDatasToVideoMem error \n");
		return -1;
	}
	
	//释放内存
	free(tPixelDatas.aucPixelDatas);
	
	return 0;
}

static void IntevalPageRun(PT_PageParams ptPageParams);

static T_PageAction g_tIntevalPageAction = {
	.name = "inteval",
	.Run  = IntevalPageRun,
};

//计算 Page Layout 位置
static void CalcIntevalPageLayout(PT_PageLayout ptPageLayout)
{
	int iXres;
	int iYres;
	int iBpp;
	int iIconWidth;
	int iIconHeight;
	int iIconMargin;
	int i;
	PT_Layout atLayout = ptPageLayout->atLayout;
	
	//测量LCD的信息
	GetDispResolution(&iXres, &iYres, &iBpp);
	ptPageLayout->iBpp = iBpp;

	// 按钮的高度是占LCD 高的2/10 ，间距是 1/10 ,宽度是 高*2
	iIconHeight = iYres * 2 / 10;
	iIconWidth  = iIconHeight * 2;
	iIconMargin = iYres * 1 / 10;

	ptPageLayout->iMaxTotalBytes = iIconWidth * iIconHeight * iBpp;

	//计算每个的大小
	i = 0;
	while(atLayout[i].strIconName)
	{
		//第3个返回图标要显示 为正方型
		switch(i)
		{
			case 0:
			{
				iIconWidth  = iYres * 2 / 10;
				iIconHeight = iIconWidth / 4;
				iIconMargin += iYres * 1 / 10;
			}break;
			case 1:
			{
				iIconWidth  = iYres * 4 / 10;
				iIconHeight = iIconWidth / 2;
				iIconMargin += iYres * 1 / 10 / 4;
			}break;
			case 2:
			{
				iIconWidth  = iYres * 2 / 10;
				iIconHeight = iIconWidth / 4;
				iIconMargin += iYres * 1 / 10 / 4;
			}break;
			case 3:
			{
				iIconWidth  = iYres * 2 / 10;
				iIconHeight = iIconWidth;
				iIconMargin += iYres * 1 / 10;
			}break;
			case 4:
			{
				iIconMargin -= iIconHeight;
			}
			break;
		}
		atLayout[i].iTopLeftX     = (iXres - iIconWidth )/2;
		atLayout[i].iBottomRightX = atLayout[i].iTopLeftX + iIconWidth;
		atLayout[i].iTopLeftY     = iIconMargin;
		atLayout[i].iBottomRightY = atLayout[i].iTopLeftY + iIconHeight;
		iIconMargin +=  iIconHeight;

		switch(i)
		{
			case 3 :
			{
				atLayout[i].iTopLeftX -= iYres * 2 / 10;
				atLayout[i].iBottomRightX -= iYres * 2 / 10;
			}
			break;
			case 4 :
			{
				atLayout[i].iTopLeftX += iYres * 2 / 10;	
				atLayout[i].iBottomRightX += iYres * 2 / 10;
			}
			break;
		}
		i++;
	}
	
}

static void ShowIntevalPage(PT_PageLayout ptPageLayout)
{
	PT_VideoMem ptVideoMem;
	PT_Layout atLayout = ptPageLayout->atLayout;
	
	//如果位置为0说明未计算位置
	if(0 == atLayout[0].iTopLeftX && 0 == atLayout[0].iBottomRightX)
	{
		CalcIntevalPageLayout(ptPageLayout);
	}
	
	//获取显存
	ptVideoMem = GetVideoMem(ID("inteval"), 1);
	if(! ptVideoMem)
	{
		DEBUG_PRINTF("cat't get video mem \n");
		return ;
	}
	
	//绘制
	if(GeneratePage(ptPageLayout, ptVideoMem) || GenerateInterval(ptVideoMem))
	{
		DEBUG_PRINTF("GeneratePage error \n");
		return ;
	}
	
	//刷到LCD
	//FlushVideoMemToDev(ptVideoMem);
	//因为重绘页面后马上重绘鼠标，所以要将显示的像素放到 镜像显存中
	FlushVideoMemToDevSync(ptVideoMem);

	//显示鼠标历史位置
	ShowHistoryMouse();
	
	//释放显存
	PutVideoMem(ptVideoMem);
	
}

static void IntevalPageRun(PT_PageParams ptPageParams)
{
	int iIndex;
	T_InputEvent tInputEvent;
	int bPressed = 0;/* 按下的状态 */
	int iIndexPressed = 0; /* 按键下标 */
	
	//显示 icons
	ShowIntevalPage(&g_tIntevalPageLayout);

	//处理事件
	/* 3. 调用GetInputEvent获得输入事件，进而处理 */
	while (1)
	{
		iIndex = GenericGetInputEvent(&g_tIntevalPageLayout, &tInputEvent);

		/**
		 *判断按键事件算法
		 *
		 */
		//松开状态 压力值 为 0
		if (tInputEvent.iPressure == 0)
		{
			if(bPressed)
			{
				/* 曾经有按钮被按下 */
				ReleaseButton(&g_atIntevalPageIconsLayout[iIndexPressed]);
				bPressed = 0;
				//如果按下和松开是同一个按键
				if(iIndexPressed == iIndex)
				{
					switch(iIndex)
					{
						case 0 : 
						{
							//修改定时数量 并重绘
							g_iInteval++;
							ReGenerateInterval();
						}
						break;

						case 1 : 
						{
							
						}
						break;

						case 2 :
						{
							//修改定时数量 并重绘
							g_iInteval = (1 >= g_iInteval) ? 1 : --g_iInteval;
							ReGenerateInterval();
						}
						break;
						case 4 :
						{
							GetPage("setting")->Run(NULL);
						}
						case 5 :
						{
							GetPage("setting")->Run(NULL);
						}
						break;
					}
				}
				iIndexPressed = -1;
			}
		}
		else
		{
			//如果按键值有效
			if(-1 != iIndex)
			{
				if(0 == bPressed)
				{
					bPressed      = 1;
					iIndexPressed = iIndex;
					PressButton(&g_atIntevalPageIconsLayout[iIndexPressed]);
				}
			}
		}
	}
}

int IntevalPageInit(void)
{
	return RegisterPageAction(&g_tIntevalPageAction);
}

