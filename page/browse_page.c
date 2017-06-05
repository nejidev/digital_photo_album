#include <conf.h>
#include <render.h>
#include <page_manager.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <file.h>
#define min(a,b) (a<b ? a : b)

/* 图标是一个正方体, "图标+名字"也是一个正方体
 *   --------
 *   |  图  |
 *   |  标  |
 * ------------
 * |   名字   |
 * ------------
 */

//当前有多少个子项目
static int g_iDirFilesNum;
//保存子项目的结构体
static PT_DirFiles *g_ptDirFiles;
//当前显示哪个目录下的子项目
static char g_acDirPath[256] = "/";

//icon图标
static T_Layout g_atBrowsePageIconsLayout[] = {
	{0, 0, 0, 0, ICON_DIR"up.bmp"},
	{0, 0, 0, 0, ICON_DIR"select.bmp"},
	{0, 0, 0, 0, ICON_DIR"pre_page.bmp"},
	{0, 0, 0, 0, ICON_DIR"next_page.bmp"},
	{0, 0, 0, 0, ICON_DIR"video_view.bmp"},    /* 实时查看 */
	{0, 0, 0, 0, ICON_DIR"video_camera.bmp"},  /* 拍照 */
	{0, 0, 0, 0, NULL},
};

//Browse Page layout
static T_PageLayout g_tBrowsePageLayout = {
	.iMaxTotalBytes = 0,
	.atLayout       = g_atBrowsePageIconsLayout,
};

//文件 layout
static T_PageLayout g_tBrowseFilesLayout = {
	.iMaxTotalBytes = 0,
};

static void BrowsePageRun(PT_PageParams ptPageParams);

static T_PageAction g_tBrowsePageAction = {
	.name = "browse",
	.Run  = BrowsePageRun,
};

//计算 Page Layout 位置
static void CalcBrowsePageLayout(PT_PageLayout ptPageLayout)
{
	int iXres;
	int iYres;
	int iBpp;
	int iIconWidth;
	int iIconHeight;
	int i;
	PT_Layout atLayout = ptPageLayout->atLayout;
	
	//测量LCD的信息
	GetDispResolution(&iXres, &iYres, &iBpp);
	ptPageLayout->iBpp = iBpp;

	// 按钮的高度是占LCD 高的2/10 一行放6个按钮
	iIconHeight = iYres * 2 / 10;
	iIconWidth  = iXres / 6;
	
	ptPageLayout->iMaxTotalBytes = iIconWidth * iIconHeight * iBpp;

	//计算每个的大小
	i = 0;
	while(atLayout[i].strIconName)
	{
		atLayout[i].iTopLeftX     = i * iIconWidth;
		atLayout[i].iBottomRightX = atLayout[i].iTopLeftX + iIconWidth;
		atLayout[i].iTopLeftY     = 0;
		atLayout[i].iBottomRightY = atLayout[i].iTopLeftY + iIconHeight;
		
		i++;
	}
	
}

static int GenerateIcon(PT_VideoMem ptVideoMem, PT_Layout ptLayout, char *pcFileName)
{
	int iXres;
	int iYres;
	int iBpp;
	T_PixelDatas tPixelDatas;
	T_PixelDatas tFontPixelDatas;
	
	//测量LCD的信息
	GetDispResolution(&iXres, &iYres, &iBpp);
	
	tPixelDatas.iHeight = DIR_FILE_ICON_HEIGHT;
	tPixelDatas.iWidth  = DIR_FILE_ICON_WIDTH;
	tPixelDatas.iBpp    = iBpp;
	tPixelDatas.iLineBytes  = tPixelDatas.iWidth * tPixelDatas.iBpp / 8;
	tPixelDatas.iTotalBytes = tPixelDatas.iLineBytes * tPixelDatas.iHeight;
	
	tPixelDatas.aucPixelDatas = malloc(tPixelDatas.iTotalBytes);
	if(! tPixelDatas.aucPixelDatas)
	{
		DEBUG_PRINTF("malloc error \n");
		return -1;
	}
	//清为背景色
	ClearPixelDatasMem(&tPixelDatas, COLOR_BACKGROUND);

	tFontPixelDatas.iHeight = DIR_FILE_NAME_HEIGHT;
	tFontPixelDatas.iWidth  = DIR_FILE_NAME_WIDTH;
	tFontPixelDatas.iBpp    = iBpp;
	tFontPixelDatas.iLineBytes  = tFontPixelDatas.iWidth * tFontPixelDatas.iBpp / 8;
	tFontPixelDatas.iTotalBytes = tFontPixelDatas.iLineBytes * tFontPixelDatas.iHeight;
	

	tFontPixelDatas.aucPixelDatas = malloc(tFontPixelDatas.iTotalBytes);
	if(! tFontPixelDatas.aucPixelDatas)
	{
		DEBUG_PRINTF("malloc error \n");
		return -1;
	}
	//清为背景色
	ClearPixelDatasMem(&tFontPixelDatas, COLOR_BACKGROUND);
	
	//获取icon 显存
	if(GetFileICON(&tPixelDatas, ptLayout->strIconName))
	{
		DEBUG_PRINTF("GetFileICON error \n");
		return -1;
	}
	//合并到主显存
	if(MergePixelDatasToVideoMem(ptLayout->iTopLeftX, ptLayout->iTopLeftY, &tPixelDatas, ptVideoMem))
	{
		DEBUG_PRINTF("MergePixelDatasToVideoMem error \n");
		return -1;
	}
	//绘制文件名 使用 freetype 或 汉字库 或 ascii 生成数据
	if(GetPixelDatasForFreetype((unsigned char *)pcFileName, &tFontPixelDatas))
	{
		DEBUG_PRINTF("GetPixelDatasForFreetype error \n");
		return -1;
	}

	//合并到主显存
	if(MergePixelDatasToVideoMem(ptLayout->iTopLeftX, ptLayout->iTopLeftY + DIR_FILE_ICON_HEIGHT , &tFontPixelDatas, ptVideoMem))
	{
		DEBUG_PRINTF("MergePixelDatasToVideoMem error \n");
		return -1;
	}

	//释放内存
	free(tPixelDatas.aucPixelDatas);
	free(tFontPixelDatas.aucPixelDatas);
	return 0;
}

static int GenerateBrowseDirIcon(PT_VideoMem ptVideoMem)
{
	int iErr = 0;
	PT_DirFiles ptDirFiles;
	T_Layout *atDirLayout;
	int dirIconX = 0;
	int dirIconY = 0;
	int iXres;
	int iYres;
	int iBpp;
	int iNum;
	int i;
	int margin = 5;

	//测量LCD的信息
	GetDispResolution(&iXres, &iYres, &iBpp);
	//读取 g_acDirPath 下的所有文件列表
	iErr = GetDirContents(g_acDirPath, &g_ptDirFiles, &g_iDirFilesNum);
	if(iErr)
	{
		DEBUG_PRINTF("GetDirContents Err %s \n", g_acDirPath);
		return iErr;
	}
	//计算可以显示多少个图标 margin:5px    减去 上部按钮占用 2/10
	iNum = (iXres / (DIR_FILE_ALL_WIDTH + margin*2)) * ((iYres * 8 / 10) / (DIR_FILE_ALL_HEIGHT + margin*2));
	
	//实际文件数量 能显示的数量 取小的
	iNum = min(iNum, g_iDirFilesNum);

	//分配layout 内存
	atDirLayout = malloc(sizeof(T_Layout) * iNum);
	g_tBrowseFilesLayout.atLayout = atDirLayout;
	
	//生成 ICON 显示
	dirIconX   = margin*2;
	dirIconY   = iYres * 2 / 10 + margin;
	while(i < iNum)
	{
		//设置图标要显示的位置及显存
		atDirLayout[i].iTopLeftX     = dirIconX;
		atDirLayout[i].iTopLeftY     = dirIconY;
		atDirLayout[i].iBottomRightX = atDirLayout[i].iTopLeftX + DIR_FILE_ALL_WIDTH;
		atDirLayout[i].iBottomRightY = atDirLayout[i].iTopLeftY + DIR_FILE_ALL_HEIGHT;
		atDirLayout[i].strIconName   = malloc(256);
		if(! atDirLayout[i].strIconName)
		{
			DEBUG_PRINTF("malloc error \n");
			return -1;
		}
		ptDirFiles = g_ptDirFiles[i];
		if(GetFileIconName(ptDirFiles, atDirLayout[i].strIconName))
		{
			DEBUG_PRINTF("GetFileIconName Err \n");
			return -1;
		}
		//绘制图标
		if(GenerateIcon(ptVideoMem, &atDirLayout[i], ptDirFiles->strName))
		{
			DEBUG_PRINTF("GenerateIcon Err \n");
			return -1;
		}
		//重新设定位置
		dirIconX += DIR_FILE_ALL_WIDTH + margin;
		//这里要计算 右X 是否超出
		if((dirIconX + DIR_FILE_ALL_WIDTH) >= iXres)
		{
			dirIconX = margin*2;
			dirIconY += DIR_FILE_ALL_HEIGHT + margin * 2;
		}
		
		i++;
	}
	
	return iErr;
}

static void ShowBrowsePage(PT_PageLayout ptPageLayout)
{
	PT_VideoMem ptVideoMem;
	PT_Layout atLayout = ptPageLayout->atLayout;
	
	//如果位置为0说明未计算位置
	if(0 == atLayout[0].iTopLeftX && 0 == atLayout[0].iBottomRightX)
	{
		CalcBrowsePageLayout(ptPageLayout);
	}
	
	//获取显存
	ptVideoMem = GetVideoMem(ID("browse"), 1);
	if(! ptVideoMem)
	{
		DEBUG_PRINTF("cat't get video mem \n");
		return ;
	}
	
	//绘制
	if(GeneratePage(ptPageLayout, ptVideoMem) || GenerateBrowseDirIcon(ptVideoMem))
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

static void BrowsePageRun(PT_PageParams ptPageParams)
{
	int iIndex;
	T_InputEvent tInputEvent;
	int bPressed = 0;/* 按下的状态 */
	int iIndexPressed = 0; /* 按键下标 */
	
	//显示 icons
	ShowBrowsePage(&g_tBrowsePageLayout);

	//处理事件
	/* 3. 调用GetInputEvent获得输入事件，进而处理 */
	while (1)
	{
		iIndex = GenericGetInputEvent(&g_tBrowsePageLayout, &tInputEvent);

		/**
		 *判断按键事件算法
		 *
		 */
		//松开状态 压力值 为 0
		if (0 == tInputEvent.iPressure)
		{
			if(bPressed)
			{
				/* 曾经有按钮被按下 */
				ReleaseButton(&g_atBrowsePageIconsLayout[iIndexPressed]);
				bPressed = 0;
				//如果按下和松开是同一个按键
				if(iIndexPressed == iIndex)
				{
					switch(iIndex)
					{
						case 0 : 
						{
						}
						break;

						case 1 : 
						{
							
						}
						break;

						case 2 :
						{
						}
						break;
						case 3 :
						{
						}
						case 4 :
						{
						
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
					PressButton(&g_atBrowsePageIconsLayout[iIndexPressed]);
				}
			}
		}
	}
}

int BroserPageInit(void)
{
	return RegisterPageAction(&g_tBrowsePageAction);
}



