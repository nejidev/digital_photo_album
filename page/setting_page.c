#include <conf.h>
#include <render.h>
#include <page_manager.h>
#include <stdlib.h>

//icon图标
static T_Layout g_atSettingPageIconsLayout[] = {
	{0, 0, 0, 0, "select_fold.bmp"},
	{0, 0, 0, 0, "interval.bmp"},
	{0, 0, 0, 0, "return.bmp"},
	{0, 0, 0, 0, NULL},
};

//Setting Page layout
static T_PageLayout g_tSettingPageLayout = {
	.iMaxTotalBytes = 0,
	.atLayout       = g_atSettingPageIconsLayout,
};

static void SettingPageRun(PT_PageParams ptPageParams);

static T_PageAction g_tSettingPageAction = {
	.name = "setting",
	.Run  = SettingPageRun,
};

//计算 Page Layout 位置
static void CalcSettingPageLayout(PT_PageLayout ptPageLayout)
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
		if(2 == i)
		{
			iIconWidth = iIconHeight;
		}
		atLayout[i].iTopLeftX     = (iXres - iIconWidth )/2;
		atLayout[i].iBottomRightX = atLayout[i].iTopLeftX + iIconWidth;
		atLayout[i].iTopLeftY     = iIconMargin;
		atLayout[i].iBottomRightY = atLayout[i].iTopLeftY + iIconHeight;
		iIconMargin +=  iIconHeight + iYres * 1 / 10;
		
		i++;
	}
	
}

static void ShowSettingPage(PT_PageLayout ptPageLayout)
{
	PT_VideoMem ptVideoMem;
	PT_Layout atLayout = ptPageLayout->atLayout;
	
	//如果位置为0说明未计算位置
	if(0 == atLayout[0].iTopLeftX && 0 == atLayout[0].iBottomRightX)
	{
		CalcSettingPageLayout(ptPageLayout);
	}
	
	//获取显存
	ptVideoMem = GetVideoMem(ID("setting"), 1);
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
	//FlushVideoMemToDev(ptVideoMem);
	//因为重绘页面后马上重绘鼠标，所以要将显示的像素放到 镜像显存中
	FlushVideoMemToDevSync(ptVideoMem);

	//显示鼠标历史位置
	ShowHistoryMouse();
	
	//释放显存
	PutVideoMem(ptVideoMem);
	
}

static void SettingPageRun(PT_PageParams ptPageParams)
{
	int iIndex;
	T_InputEvent tInputEvent;
	int bPressed = 0;/* 按下的状态 */
	int iIndexPressed = 0; /* 按键下标 */
	
	//显示 icons
	ShowSettingPage(&g_tSettingPageLayout);

	//处理事件
	/* 3. 调用GetInputEvent获得输入事件，进而处理 */
	while (1)
	{
		iIndex = GenericGetInputEvent(&g_tSettingPageLayout, &tInputEvent);

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
				ReleaseButton(&g_atSettingPageIconsLayout[iIndexPressed]);
				bPressed = 0;
				//如果按下和松开是同一个按键
				if(iIndexPressed == iIndex)
				{
					switch(iIndex)
					{
						case 0 : 
						break;

						case 1 : 
						{
							GetPage("inteval")->Run(NULL);
						}
						break;

						case 2 :
						{
							GetPage("main")->Run(NULL);
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
					PressButton(&g_atSettingPageIconsLayout[iIndexPressed]);
				}
			}
		}
	}
}

int SettingPageInit(void)
{
	return RegisterPageAction(&g_tSettingPageAction);
}

