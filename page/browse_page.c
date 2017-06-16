#include <conf.h>
#include <render.h>
#include <page_manager.h>
#include <disp_manager.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <file.h>
#include <libgen.h>
#include <unistd.h>

#define min(a,b) (a<b ? a : b)

/* 图标是一个正方体, "图标+名字"也是一个正方体
 *   --------
 *   |  图  |
 *   |  标  |
 * ------------
 * |   名字   |
 * ------------
 */
//图标的间距
#define ICON_MARGIN 10
//当前是否有显示进程占用主区域
static int g_iUseMainArea = 0;
//是否正在播放 GIF
static int g_iGIFPlay      = 0;
//当前有多少个子项目
static int g_iDirFilesNum;
//当前页显示多少个子项目
static int g_iShowFilesNum;
//当前显示的位置
static int g_iDirFilesIndex = 0;
//每页显示的最大数量
static int g_iShowMaxFilesNum = 0;
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

static int GenerateIcon(PT_VideoMem ptVideoMem, PT_Layout ptLayout, char *pcFileName);
static int GenerateBrowseDirIcon(PT_VideoMem ptVideoMem);
static int BrowseDir(void);

//运行 nes 模拟器 
/**
 * 使用说明:必须先在文件系统中编译 InfoNES 项目 并把 InfoNES 可执行文件
 * 添加到 /usr/bin 可执行PATH 中，而且内核要支持 ALSA 音频，否则没声音
 * 手柄支持USB 和 真实手柄2种，并且也需要内核支持和键位对应
 */
static int RunNesGame(char *nes)
{
	char NesPath[256];
	PT_DispOpr disp;
	disp = GetDefaultDispDev();
	//因为 InfoNes 也需要绘制在 lcd 上所以要先关闭 数码相册
	disp->DeviceExit();
	snprintf(NesPath, 256, "%s %s/%s", "InfoNES", g_acDirPath, nes);
	if(-1 == system(NesPath))
	{
		DEBUG_PRINTF("exec InfoNES %s err\n", NesPath);
		disp->DeviceInit();
		return -1;
	}
	return 0;
}

static int RunGIFPlay(char *file)
{
	char filePath[256];
	
	snprintf(filePath, 256, "%s/%s", g_acDirPath, file);
	//1,检查是不是GIF文件
	if(GIFisSupport(filePath))
	{
		DEBUG_PRINTF("GIFisSupport err \n");
		return -1;
	}
	
	//2,启动播放进程
	GIFPlayStart();
	//占用主区域 为事件处理做准备 在次点击图片返回
	g_iUseMainArea++;
	g_iGIFPlay++;
	return 0;
}

//查看 BMP 图片
static int RunBMPJPG(char *file)
{
	char filePath[256];
	int iXres;
	int iYres;
	int iBpp;
	PT_VideoMem ptVideoMem;
	//读取Icon图片缩放到合适大小
	T_PixelDatas tOriginIconPixelDatas;
 	T_PixelDatas tIconPixelDatas;
	
	ptVideoMem = GetVideoMem(ID("sync"), 1);
	
	//测量LCD的信息
	GetDispResolution(&iXres, &iYres, &iBpp);
	snprintf(filePath, 256, "%s/%s", g_acDirPath, file);
	
	//设置缩放大小
	tOriginIconPixelDatas.iBpp  = iBpp;
	tIconPixelDatas.iBpp        = iBpp;
	tIconPixelDatas.iHeight     = iYres * 8 / 10;
	tIconPixelDatas.iWidth      = iXres;
	tIconPixelDatas.iLineBytes  = tIconPixelDatas.iWidth * tIconPixelDatas.iBpp / 8;
	tIconPixelDatas.iTotalBytes = tIconPixelDatas.iLineBytes * tIconPixelDatas.iHeight;
	

	//分配缩放后 icon 图片的显存
	tIconPixelDatas.aucPixelDatas = (unsigned char *)malloc(tIconPixelDatas.iTotalBytes);
	if(NULL == tIconPixelDatas.aucPixelDatas)
	{
		DEBUG_PRINTF("malloc tIconPixelDatas error \n");
		return -1;
	}
	//得到 BMP 图片数据
	if(GetPixelDatasForIcon(filePath, &tOriginIconPixelDatas))
	{
		free(tIconPixelDatas.aucPixelDatas);
		DEBUG_PRINTF("GetPixelDatasForIcon error \n");
		return -1;
	}
	//执行缩放
	PicZoom(&tOriginIconPixelDatas, &tIconPixelDatas);
			
	//写入显存
	PicMerge(0, iYres * 2 / 10, &tIconPixelDatas, &ptVideoMem->tPixelDatas);
			
	//释放 BMP 图片数据内存
	free(tOriginIconPixelDatas.aucPixelDatas);
	free(tIconPixelDatas.aucPixelDatas);
	
	FlushVideoMemToDevSync(ptVideoMem);
	ShowHistoryMouse();

	//占用主区域 为事件处理做准备 在次点击图片返回
	g_iUseMainArea++;
	return 0;
}

//清除重绘区域
static int CleanBrowseMainArea(PT_VideoMem ptVideoMem)
{
	int x = 0;
	int y = 0;
	int iXres;
	int iYres;
	int iBpp;

	//测量LCD的信息
	GetDispResolution(&iXres, &iYres, &iBpp);
	
	//X开始位置 0 Y 开始位置 2/10
	y = iYres * 2 / 10;
	for(; y<iYres; y++)
	{
		for(x=0; x<iXres; x++)
		{
			ShowPixelPixelDatasMem(&ptVideoMem->tPixelDatas, x, y, COLOR_BACKGROUND);
		}
	}
	return 0;
}

static int BrowseDir(void)
{
	PT_VideoMem ptVideoMem;
	ptVideoMem = GetVideoMem(ID("sync"), 1);
	//切换目录后 重置分页索引
	g_iDirFilesIndex = 0;
	//清为背景色
	CleanBrowseMainArea(ptVideoMem);
	if(GenerateBrowseDirIcon(ptVideoMem))
	{
		DEBUG_PRINTF("GenerateBrowseDirIcon error \n");	
		return -1;
	}
	FlushVideoMemToDevSync(ptVideoMem);
	ShowHistoryMouse();
	
	return 0;
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

//处理文件图标点击
static int RunIconEvent(int iEventID, PT_InputEvent ptInputEvent)
{
	char *IconName; 
	PT_DirFiles ptDirFiles;
	PT_VideoMem ptVideoMem;
	PT_Layout ptLayout;
	char JumpDirPath[256];
	
	ptVideoMem = GetVideoMem(ID("sync"), 1);	
	
	/* 如果是按下 */
	if(1 == ptInputEvent->iPressure)
	{
		if(-1 < iEventID)
		{
			ptLayout = &g_tBrowseFilesLayout.atLayout[iEventID];
			IconName = basename(ptLayout->strIconName);	
			//找到对应的文件
			ptDirFiles = g_ptDirFiles[g_iDirFilesIndex - g_iShowFilesNum + iEventID];
			
			//切换目录
			if(0 == strcmp("dir.bmp", IconName))
			{
				//更换文件打开的图标
				ptLayout->strIconName = ICON_DIR"dir_open.bmp";
				if(GenerateIcon(ptVideoMem, ptLayout, ptDirFiles->strName))
				{
					DEBUG_PRINTF("GenerateIcon error \n");	
					return -1;
				}
				FlushVideoMemToDevSync(ptVideoMem);
				ShowHistoryMouse();
				
				//设置当前路径
				snprintf(JumpDirPath, 256, "%s/%s", g_acDirPath, ptDirFiles->strName);
				realpath(JumpDirPath, g_acDirPath);
				//显示
				BrowseDir();
			}
			//运行nes 游戏
			if(0 == strcmp("nes.bmp", IconName))
			{
				if(RunNesGame(ptDirFiles->strName))
				{
					DEBUG_PRINTF("RunNesGame error \n");	
					return -1;
				}
			}
			//查看 bmp jpg 图片
			if(0 == strcmp("bmp.bmp", IconName) || 0 == strcmp("jpg.bmp", IconName))
			{
				if(RunBMPJPG(ptDirFiles->strName))
				{
					DEBUG_PRINTF("RunBMPJPG error \n");	
					return -1;
				}
			}
			//运行gif 播放
			if(0 == strcmp("gif.bmp", IconName))
			{
				if(RunGIFPlay(ptDirFiles->strName))
				{
					DEBUG_PRINTF("RunGIFPlay error \n");	
					return -1;
				}
			}
		}
	}
	PutVideoMem(ptVideoMem);
	return 0;
}

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

static int GenerateBrowseDirIcon(PT_VideoMem ptVideoMem)
{
	int iErr = 0;
	PT_DirFiles ptDirFiles;
	int dirIconX = 0;
	int dirIconY = 0;
	int iXres;
	int iYres;
	int iBpp;
	int iNum;
	int i;

	//测量LCD的信息
	GetDispResolution(&iXres, &iYres, &iBpp);
	//读取 g_acDirPath 下的所有文件列表
	iErr = GetDirContents(g_acDirPath, &g_ptDirFiles, &g_iDirFilesNum);
	if(iErr)
	{
		DEBUG_PRINTF("GetDirContents Err %s \n", g_acDirPath);
		return iErr;
	}
	//计算可以显示多少个图标 ICON_MARGIN    减去 上部按钮占用 2/10
	g_iShowMaxFilesNum = iNum = (iXres / (DIR_FILE_ALL_WIDTH + ICON_MARGIN*2)) * ((iYres * 8 / 10) / (DIR_FILE_ALL_HEIGHT + ICON_MARGIN));
	
	//实际文件数量 能显示的数量 取小的
	iNum = min(iNum, g_iDirFilesNum);
	
	//分配layout 内存
	if(! g_tBrowseFilesLayout.atLayout)
	{
		free(g_tBrowseFilesLayout.atLayout);
	}
	g_tBrowseFilesLayout.atLayout = malloc(sizeof(T_Layout) * iNum);
	if(! g_tBrowseFilesLayout.atLayout)
	{
		DEBUG_PRINTF("malloc g_tBrowseFilesLayout.atLayout error \n");
		return 0;
	}
	
	//生成 ICON 显示
	dirIconX   = ICON_MARGIN*2;
	dirIconY   = iYres * 2 / 10 + ICON_MARGIN;
	i = 0;
	while(i < iNum)
	{
		//设置图标要显示的位置及显存
		g_tBrowseFilesLayout.atLayout[i].iTopLeftX     = dirIconX;
		g_tBrowseFilesLayout.atLayout[i].iTopLeftY     = dirIconY;
		g_tBrowseFilesLayout.atLayout[i].iBottomRightX = g_tBrowseFilesLayout.atLayout[i].iTopLeftX + DIR_FILE_ALL_WIDTH;
		g_tBrowseFilesLayout.atLayout[i].iBottomRightY = g_tBrowseFilesLayout.atLayout[i].iTopLeftY + DIR_FILE_ALL_HEIGHT;
		g_tBrowseFilesLayout.atLayout[i].strIconName   = malloc(256);
		if(! g_tBrowseFilesLayout.atLayout[i].strIconName)
		{
			DEBUG_PRINTF("malloc error \n");
			return -1;
		}
		ptDirFiles = g_ptDirFiles[i];
		if(GetFileIconName(ptDirFiles, g_tBrowseFilesLayout.atLayout[i].strIconName))
		{
			DEBUG_PRINTF("GetFileIconName Err \n");
			return -1;
		}
		//绘制图标
		if(GenerateIcon(ptVideoMem, &g_tBrowseFilesLayout.atLayout[i], ptDirFiles->strName))
		{
			DEBUG_PRINTF("GenerateIcon Err \n");
			return -1;
		}
		//重新设定位置
		dirIconX += DIR_FILE_ALL_WIDTH + ICON_MARGIN;
		//这里要计算 右X 是否超出
		if((dirIconX + DIR_FILE_ALL_WIDTH) >= iXres)
		{
			dirIconX = ICON_MARGIN*2;
			dirIconY += DIR_FILE_ALL_HEIGHT + ICON_MARGIN;
		}
		i++;
	}
	g_iDirFilesIndex += iNum;
	g_iShowFilesNum   = iNum;
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

//显示上一页
static int ShowBrowsePrePage(void)
{
	int iNum = 0;
	int i;
	int iXres;
	int iYres;
	int iBpp;
	int dirIconX,dirIconY;
	PT_DirFiles ptDirFiles;
	PT_VideoMem ptVideoMem;

	//计算上一页的显示索引
	g_iDirFilesIndex -= (g_iShowMaxFilesNum + g_iShowFilesNum);
	if(0 > g_iDirFilesIndex)
	{
		g_iDirFilesIndex = 0;
	}
	//测量LCD的信息
	GetDispResolution(&iXres, &iYres, &iBpp);
	//获取镜像显存
	ptVideoMem = GetVideoMem(ID("sync"), 1);
	//清为背景色
	CleanBrowseMainArea(ptVideoMem);

	//分配layout 内存
	if(! g_tBrowseFilesLayout.atLayout)
	{
		free(g_tBrowseFilesLayout.atLayout);
	}
	
	//计算可以显示多少个图标: 上一页肯定是满屏
	iNum = g_iShowMaxFilesNum;
	g_tBrowseFilesLayout.atLayout = malloc(sizeof(T_Layout) * iNum);
	if(! g_tBrowseFilesLayout.atLayout)
	{
		DEBUG_PRINTF("malloc g_tBrowseFilesLayout.atLayout error \n");
		return 0;
	}
	
	//生成 ICON 显示
	dirIconX   = ICON_MARGIN*2;
	dirIconY   = iYres * 2 / 10 + ICON_MARGIN;
	i = 0;
	while(i < iNum)
	{
		//设置图标要显示的位置及显存
		g_tBrowseFilesLayout.atLayout[i].iTopLeftX     = dirIconX;
		g_tBrowseFilesLayout.atLayout[i].iTopLeftY     = dirIconY;
		g_tBrowseFilesLayout.atLayout[i].iBottomRightX = g_tBrowseFilesLayout.atLayout[i].iTopLeftX + DIR_FILE_ALL_WIDTH;
		g_tBrowseFilesLayout.atLayout[i].iBottomRightY = g_tBrowseFilesLayout.atLayout[i].iTopLeftY + DIR_FILE_ALL_HEIGHT;
		g_tBrowseFilesLayout.atLayout[i].strIconName   = malloc(256);
		if(! g_tBrowseFilesLayout.atLayout[i].strIconName)
		{
			DEBUG_PRINTF("malloc error \n");
			return -1;
		}
		ptDirFiles = g_ptDirFiles[g_iDirFilesIndex + i];
		if(GetFileIconName(ptDirFiles, g_tBrowseFilesLayout.atLayout[i].strIconName))
		{
			DEBUG_PRINTF("GetFileIconName Err \n");
			return -1;
		}
		//绘制图标
		if(GenerateIcon(ptVideoMem, &g_tBrowseFilesLayout.atLayout[i], ptDirFiles->strName))
		{
			DEBUG_PRINTF("GenerateIcon Err \n");
			return -1;
		}
		//重新设定位置
		dirIconX += DIR_FILE_ALL_WIDTH + ICON_MARGIN;
		//这里要计算 右X 是否超出
		if((dirIconX + DIR_FILE_ALL_WIDTH) >= iXres)
		{
			dirIconX = ICON_MARGIN*2;
			dirIconY += DIR_FILE_ALL_HEIGHT + ICON_MARGIN;
		}
		i++;
	}
	g_iDirFilesIndex += iNum;
	g_iShowFilesNum = iNum;

	FlushVideoMemToDevSync(ptVideoMem);
	ShowHistoryMouse();

	return 0;
}

//显示下一页
static int ShowBrowseNextPage(void)
{
	int iNum = 0;
	int i;
	int iXres;
	int iYres;
	int iBpp;
	int dirIconX,dirIconY;
	PT_DirFiles ptDirFiles;
	PT_VideoMem ptVideoMem;

	//计算可以显示多少个图标: min(每页最多显示数量, 剩余数量)
	iNum = min(g_iShowMaxFilesNum, (g_iDirFilesNum - g_iDirFilesIndex));
	if(1 > iNum)
	{
		DEBUG_PRINTF("ShowBrowseNextPage last page\n");
		return -1;
	}
	
	//测量LCD的信息
	GetDispResolution(&iXres, &iYres, &iBpp);
	//获取镜像显存
	ptVideoMem = GetVideoMem(ID("sync"), 1);
	
	//清为背景色
	CleanBrowseMainArea(ptVideoMem);
	
	//分配layout 内存
	if(! g_tBrowseFilesLayout.atLayout)
	{
		free(g_tBrowseFilesLayout.atLayout);
	}
	g_tBrowseFilesLayout.atLayout = malloc(sizeof(T_Layout) * iNum);
	if(! g_tBrowseFilesLayout.atLayout)
	{
		DEBUG_PRINTF("malloc g_tBrowseFilesLayout.atLayout error \n");
		return 0;
	}
	
	//生成 ICON 显示
	dirIconX   = ICON_MARGIN*2;
	dirIconY   = iYres * 2 / 10 + ICON_MARGIN;
	i = 0;
	while(i < iNum)
	{
		//设置图标要显示的位置及显存
		g_tBrowseFilesLayout.atLayout[i].iTopLeftX     = dirIconX;
		g_tBrowseFilesLayout.atLayout[i].iTopLeftY     = dirIconY;
		g_tBrowseFilesLayout.atLayout[i].iBottomRightX = g_tBrowseFilesLayout.atLayout[i].iTopLeftX + DIR_FILE_ALL_WIDTH;
		g_tBrowseFilesLayout.atLayout[i].iBottomRightY = g_tBrowseFilesLayout.atLayout[i].iTopLeftY + DIR_FILE_ALL_HEIGHT;
		g_tBrowseFilesLayout.atLayout[i].strIconName   = malloc(256);
		if(! g_tBrowseFilesLayout.atLayout[i].strIconName)
		{
			DEBUG_PRINTF("malloc error \n");
			return -1;
		}
		ptDirFiles = g_ptDirFiles[g_iDirFilesIndex + i];
		if(GetFileIconName(ptDirFiles, g_tBrowseFilesLayout.atLayout[i].strIconName))
		{
			DEBUG_PRINTF("GetFileIconName Err \n");
			return -1;
		}
		//绘制图标
		if(GenerateIcon(ptVideoMem, &g_tBrowseFilesLayout.atLayout[i], ptDirFiles->strName))
		{
			DEBUG_PRINTF("GenerateIcon Err \n");
			return -1;
		}
		//重新设定位置
		dirIconX += DIR_FILE_ALL_WIDTH + ICON_MARGIN;
		//这里要计算 右X 是否超出
		if((dirIconX + DIR_FILE_ALL_WIDTH) >= iXres)
		{
			dirIconX = ICON_MARGIN*2;
			dirIconY += DIR_FILE_ALL_HEIGHT + ICON_MARGIN;
		}
		i++;
	}
	g_iDirFilesIndex += iNum;
	g_iShowFilesNum   = iNum;

	FlushVideoMemToDevSync(ptVideoMem);
	ShowHistoryMouse();
	return 0;
}

static void BrowsePageRun(PT_PageParams ptPageParams)
{
	int iIndex;
	T_InputEvent tInputEvent;
	int bPressed = 0;/* 按下的状态 */
	int iIndexPressed = 0; /* 按键下标 */
	char JumpDirPath[256];
	
	//显示 icons
	ShowBrowsePage(&g_tBrowsePageLayout);

	//处理事件
	/* 3. 调用GetInputEvent获得输入事件，进而处理 */
	while (1)
	{
		iIndex = GenericGetInputEvent(&g_tBrowsePageLayout, &tInputEvent);
		//在生成的文件图标中查找
		if(-1 == iIndex)
		{
			//如果有其它绘图占了主区域 在次点击它时就返回
			if(g_iUseMainArea)
			{
				if(g_iGIFPlay)
				{
					g_iGIFPlay = 0;
					GIFPlayStop();
				}
				//重绘当前路径
				BrowseDir();
				g_iUseMainArea = 0;
			}
			iIndex = GenericGetInputEvent(&g_tBrowseFilesLayout, &tInputEvent);
			if(RunIconEvent(iIndex, &tInputEvent))
			{
				DEBUG_PRINTF("RunIconEvent error \n");
			}
			continue;
		}

		/**
		 *判断按键事件算法
		 *
		 */
		//松开状态 压力值 为 0
		if (0 == tInputEvent.iPressure)
		{
			if(bPressed)
			{
				bPressed = 0;
				//如果按下和松开是同一个按键
				if(iIndexPressed == iIndex)
				{
					switch(iIndex)
					{
						case 0 : 
						{
							//向上浏览路径
							snprintf(JumpDirPath, 256, "%s/%s", g_acDirPath, "..");
							realpath(JumpDirPath, g_acDirPath);
							BrowseDir();
						}
						break;

						case 1 : 
						{
							//选择
						}
						break;

						case 2 :
						{
							//上一页
							ShowBrowsePrePage();
						}
						break;
						case 3 :
						{
							//下一页
							ShowBrowseNextPage();
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
				}
			}
		}
	}
}

int BroserPageInit(void)
{
	return RegisterPageAction(&g_tBrowsePageAction);
}


