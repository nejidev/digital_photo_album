#include <conf.h>
#include <render.h>
#include <page_manager.h>
#include <stdlib.h>

//icon图标
static T_Layout g_atMainPageIconsLayout[] = {
	{0, 0, 0, 0, "browse_mode.bmp"},
	{0, 0, 0, 0, "continue_mod.bmp"},
	{0, 0, 0, 0, "setting.bmp"},
	{0, 0, 0, 0, NULL},
};

//main Page layout
static T_PageLayout g_tMainPageLayout = {
	.iMaxTotalBytes = 0,
	.atLayout       = g_atMainPageIconsLayout,
};

static void MainPageRun(PT_PageParams ptPageParams);

static T_PageAction g_tMainPageAction = {
	.name = "main",
	.Run  = MainPageRun,
};

//计算 Page Layout 位置
static void CalcMainPageLayout(PT_PageLayout ptPageLayout)
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
	while(atLayout[i].strIconName)
	{
		atLayout[i].iTopLeftX     = (iXres - iIconWidth )/2;
		atLayout[i].iBottomRightX = atLayout[i].iTopLeftX + iIconWidth;
		atLayout[i].iTopLeftY     = iIconMargin;
		atLayout[i].iBottomRightY = atLayout[i].iTopLeftY + iIconHeight;
		iIconMargin +=  iIconHeight + iYres * 1 / 10;
		
		i++;
	}
	
}

static void ShowMainPage(PT_PageLayout ptPageLayout)
{
	PT_VideoMem ptVideoMem;
	PT_Layout atLayout = ptPageLayout->atLayout;
	
	//如果位置为0说明未计算位置
	if(0 == atLayout[0].iTopLeftX && 0 == atLayout[0].iBottomRightX)
	{
		CalcMainPageLayout(ptPageLayout);
	}
	
	//获取显存
	ptVideoMem = GetVideoMem(ID("main"), 1);
	if(! ptVideoMem)
	{
		DEBUG_PRINTF("cat't get video mem \n");
		return ;
	}
	
	//绘制
	if(GeneratePage(ptPageLayout, ptVideoMem))
	{
		DEBUG_PRINTF("GeneratePage error \n");
		return ;
	}
	
	//刷到LCD
	FlushVideoMemToDev(ptVideoMem);

	//显示鼠标历史位置
	ShowHistoryMouse();
	
	//释放显存
	PutVideoMem(ptVideoMem);
	
}

static void MainPageRun(PT_PageParams ptPageParams)
{
	int iIndex;
	T_InputEvent tInputEvent;
	T_PageParams tPageParams;
	tPageParams.iPageID = ID("main");
	int bPressed = 0;/* 按下的状态 */
	int iIndexPressed = 0; /* 按键下标 */
	
	//显示 icons
	ShowMainPage(&g_tMainPageLayout);

	//处理事件
	/* 3. 调用GetInputEvent获得输入事件，进而处理 */
	while (1)
	{
		iIndex = GenericGetInputEvent(&g_tMainPageLayout, &tInputEvent);

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
				ReleaseButton(&g_atMainPageIconsLayout[iIndexPressed]);
				bPressed = 0;
				//如果按下和松开是同一个按键
				if(iIndexPressed == iIndex)
				{
					switch(iIndex)
					{
						case 1 : 
						break;

						case 2 : 
						break;

						case 3 : 
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
					PressButton(&g_atMainPageIconsLayout[iIndexPressed]);
				}
			}
		}
	}
}

int MainPageInit(void)
{
	return RegisterPageAction(&g_tMainPageAction);
}

