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

/* ͼ����һ��������, "ͼ��+����"Ҳ��һ��������
 *   --------
 *   |  ͼ  |
 *   |  ��  |
 * ------------
 * |   ����   |
 * ------------
 */
//ͼ��ļ��
#define ICON_MARGIN 10
//��ǰ�Ƿ�����ʾ����ռ��������
static int g_iUseMainArea = 0;
//�Ƿ����ڲ��� GIF
static int g_iGIFPlay      = 0;
//��ǰ�ж��ٸ�����Ŀ
static int g_iDirFilesNum;
//��ǰҳ��ʾ���ٸ�����Ŀ
static int g_iShowFilesNum;
//��ǰ��ʾ��λ��
static int g_iDirFilesIndex = 0;
//ÿҳ��ʾ���������
static int g_iShowMaxFilesNum = 0;
//��������Ŀ�Ľṹ��
static PT_DirFiles *g_ptDirFiles;
//��ǰ��ʾ�ĸ�Ŀ¼�µ�����Ŀ
static char g_acDirPath[256] = "/";

//iconͼ��
static T_Layout g_atBrowsePageIconsLayout[] = {
	{0, 0, 0, 0, ICON_DIR"up.bmp"},
	{0, 0, 0, 0, ICON_DIR"select.bmp"},
	{0, 0, 0, 0, ICON_DIR"pre_page.bmp"},
	{0, 0, 0, 0, ICON_DIR"next_page.bmp"},
	{0, 0, 0, 0, ICON_DIR"video_view.bmp"},    /* ʵʱ�鿴 */
	{0, 0, 0, 0, ICON_DIR"video_camera.bmp"},  /* ���� */
	{0, 0, 0, 0, NULL},
};

//Browse Page layout
static T_PageLayout g_tBrowsePageLayout = {
	.iMaxTotalBytes = 0,
	.atLayout       = g_atBrowsePageIconsLayout,
};

//�ļ� layout
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

//���� nes ģ���� 
/**
 * ʹ��˵��:���������ļ�ϵͳ�б��� InfoNES ��Ŀ ���� InfoNES ��ִ���ļ�
 * ��ӵ� /usr/bin ��ִ��PATH �У������ں�Ҫ֧�� ALSA ��Ƶ������û����
 * �ֱ�֧��USB �� ��ʵ�ֱ�2�֣�����Ҳ��Ҫ�ں�֧�ֺͼ�λ��Ӧ
 */
static int RunNesGame(char *nes)
{
	char NesPath[256];
	PT_DispOpr disp;
	disp = GetDefaultDispDev();
	//��Ϊ InfoNes Ҳ��Ҫ������ lcd ������Ҫ�ȹر� �������
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
	//1,����ǲ���GIF�ļ�
	if(GIFisSupport(filePath))
	{
		DEBUG_PRINTF("GIFisSupport err \n");
		return -1;
	}
	
	//2,�������Ž���
	GIFPlayStart();
	//ռ�������� Ϊ�¼�������׼�� �ڴε��ͼƬ����
	g_iUseMainArea++;
	g_iGIFPlay++;
	return 0;
}

//�鿴 BMP ͼƬ
static int RunBMPJPG(char *file)
{
	char filePath[256];
	int iXres;
	int iYres;
	int iBpp;
	PT_VideoMem ptVideoMem;
	//��ȡIconͼƬ���ŵ����ʴ�С
	T_PixelDatas tOriginIconPixelDatas;
 	T_PixelDatas tIconPixelDatas;
	
	ptVideoMem = GetVideoMem(ID("sync"), 1);
	
	//����LCD����Ϣ
	GetDispResolution(&iXres, &iYres, &iBpp);
	snprintf(filePath, 256, "%s/%s", g_acDirPath, file);
	
	//�������Ŵ�С
	tOriginIconPixelDatas.iBpp  = iBpp;
	tIconPixelDatas.iBpp        = iBpp;
	tIconPixelDatas.iHeight     = iYres * 8 / 10;
	tIconPixelDatas.iWidth      = iXres;
	tIconPixelDatas.iLineBytes  = tIconPixelDatas.iWidth * tIconPixelDatas.iBpp / 8;
	tIconPixelDatas.iTotalBytes = tIconPixelDatas.iLineBytes * tIconPixelDatas.iHeight;
	

	//�������ź� icon ͼƬ���Դ�
	tIconPixelDatas.aucPixelDatas = (unsigned char *)malloc(tIconPixelDatas.iTotalBytes);
	if(NULL == tIconPixelDatas.aucPixelDatas)
	{
		DEBUG_PRINTF("malloc tIconPixelDatas error \n");
		return -1;
	}
	//�õ� BMP ͼƬ����
	if(GetPixelDatasForIcon(filePath, &tOriginIconPixelDatas))
	{
		free(tIconPixelDatas.aucPixelDatas);
		DEBUG_PRINTF("GetPixelDatasForIcon error \n");
		return -1;
	}
	//ִ������
	PicZoom(&tOriginIconPixelDatas, &tIconPixelDatas);
			
	//д���Դ�
	PicMerge(0, iYres * 2 / 10, &tIconPixelDatas, &ptVideoMem->tPixelDatas);
			
	//�ͷ� BMP ͼƬ�����ڴ�
	free(tOriginIconPixelDatas.aucPixelDatas);
	free(tIconPixelDatas.aucPixelDatas);
	
	FlushVideoMemToDevSync(ptVideoMem);
	ShowHistoryMouse();

	//ռ�������� Ϊ�¼�������׼�� �ڴε��ͼƬ����
	g_iUseMainArea++;
	return 0;
}

//����ػ�����
static int CleanBrowseMainArea(PT_VideoMem ptVideoMem)
{
	int x = 0;
	int y = 0;
	int iXres;
	int iYres;
	int iBpp;

	//����LCD����Ϣ
	GetDispResolution(&iXres, &iYres, &iBpp);
	
	//X��ʼλ�� 0 Y ��ʼλ�� 2/10
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
	//�л�Ŀ¼�� ���÷�ҳ����
	g_iDirFilesIndex = 0;
	//��Ϊ����ɫ
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
	
	//����LCD����Ϣ
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
	//��Ϊ����ɫ
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
	//��Ϊ����ɫ
	ClearPixelDatasMem(&tFontPixelDatas, COLOR_BACKGROUND);
	
	//��ȡicon �Դ�
	if(GetFileICON(&tPixelDatas, ptLayout->strIconName))
	{
		DEBUG_PRINTF("GetFileICON error \n");
		return -1;
	}
	//�ϲ������Դ�
	if(MergePixelDatasToVideoMem(ptLayout->iTopLeftX, ptLayout->iTopLeftY, &tPixelDatas, ptVideoMem))
	{
		DEBUG_PRINTF("MergePixelDatasToVideoMem error \n");
		return -1;
	}
	
	//�����ļ��� ʹ�� freetype �� ���ֿ� �� ascii ��������
	if(GetPixelDatasForFreetype((unsigned char *)pcFileName, &tFontPixelDatas))
	{
		DEBUG_PRINTF("GetPixelDatasForFreetype error \n");
		return -1;
	}
	//�ϲ������Դ�
	if(MergePixelDatasToVideoMem(ptLayout->iTopLeftX, ptLayout->iTopLeftY + DIR_FILE_ICON_HEIGHT , &tFontPixelDatas, ptVideoMem))
	{
		DEBUG_PRINTF("MergePixelDatasToVideoMem error \n");
		return -1;
	}
	
	//�ͷ��ڴ�
	free(tPixelDatas.aucPixelDatas);
	free(tFontPixelDatas.aucPixelDatas);
	return 0;
}

//�����ļ�ͼ����
static int RunIconEvent(int iEventID, PT_InputEvent ptInputEvent)
{
	char *IconName; 
	PT_DirFiles ptDirFiles;
	PT_VideoMem ptVideoMem;
	PT_Layout ptLayout;
	char JumpDirPath[256];
	
	ptVideoMem = GetVideoMem(ID("sync"), 1);	
	
	/* ����ǰ��� */
	if(1 == ptInputEvent->iPressure)
	{
		if(-1 < iEventID)
		{
			ptLayout = &g_tBrowseFilesLayout.atLayout[iEventID];
			IconName = basename(ptLayout->strIconName);	
			//�ҵ���Ӧ���ļ�
			ptDirFiles = g_ptDirFiles[g_iDirFilesIndex - g_iShowFilesNum + iEventID];
			
			//�л�Ŀ¼
			if(0 == strcmp("dir.bmp", IconName))
			{
				//�����ļ��򿪵�ͼ��
				ptLayout->strIconName = ICON_DIR"dir_open.bmp";
				if(GenerateIcon(ptVideoMem, ptLayout, ptDirFiles->strName))
				{
					DEBUG_PRINTF("GenerateIcon error \n");	
					return -1;
				}
				FlushVideoMemToDevSync(ptVideoMem);
				ShowHistoryMouse();
				
				//���õ�ǰ·��
				snprintf(JumpDirPath, 256, "%s/%s", g_acDirPath, ptDirFiles->strName);
				realpath(JumpDirPath, g_acDirPath);
				//��ʾ
				BrowseDir();
			}
			//����nes ��Ϸ
			if(0 == strcmp("nes.bmp", IconName))
			{
				if(RunNesGame(ptDirFiles->strName))
				{
					DEBUG_PRINTF("RunNesGame error \n");	
					return -1;
				}
			}
			//�鿴 bmp jpg ͼƬ
			if(0 == strcmp("bmp.bmp", IconName) || 0 == strcmp("jpg.bmp", IconName))
			{
				if(RunBMPJPG(ptDirFiles->strName))
				{
					DEBUG_PRINTF("RunBMPJPG error \n");	
					return -1;
				}
			}
			//����gif ����
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

//���� Page Layout λ��
static void CalcBrowsePageLayout(PT_PageLayout ptPageLayout)
{
	int iXres;
	int iYres;
	int iBpp;
	int iIconWidth;
	int iIconHeight;
	int i;
	PT_Layout atLayout = ptPageLayout->atLayout;
	
	//����LCD����Ϣ
	GetDispResolution(&iXres, &iYres, &iBpp);
	ptPageLayout->iBpp = iBpp;

	// ��ť�ĸ߶���ռLCD �ߵ�2/10 һ�з�6����ť
	iIconHeight = iYres * 2 / 10;
	iIconWidth  = iXres / 6;
	
	ptPageLayout->iMaxTotalBytes = iIconWidth * iIconHeight * iBpp;

	//����ÿ���Ĵ�С
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

	//����LCD����Ϣ
	GetDispResolution(&iXres, &iYres, &iBpp);
	//��ȡ g_acDirPath �µ������ļ��б�
	iErr = GetDirContents(g_acDirPath, &g_ptDirFiles, &g_iDirFilesNum);
	if(iErr)
	{
		DEBUG_PRINTF("GetDirContents Err %s \n", g_acDirPath);
		return iErr;
	}
	//���������ʾ���ٸ�ͼ�� ICON_MARGIN    ��ȥ �ϲ���ťռ�� 2/10
	g_iShowMaxFilesNum = iNum = (iXres / (DIR_FILE_ALL_WIDTH + ICON_MARGIN*2)) * ((iYres * 8 / 10) / (DIR_FILE_ALL_HEIGHT + ICON_MARGIN));
	
	//ʵ���ļ����� ����ʾ������ ȡС��
	iNum = min(iNum, g_iDirFilesNum);
	
	//����layout �ڴ�
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
	
	//���� ICON ��ʾ
	dirIconX   = ICON_MARGIN*2;
	dirIconY   = iYres * 2 / 10 + ICON_MARGIN;
	i = 0;
	while(i < iNum)
	{
		//����ͼ��Ҫ��ʾ��λ�ü��Դ�
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
		//����ͼ��
		if(GenerateIcon(ptVideoMem, &g_tBrowseFilesLayout.atLayout[i], ptDirFiles->strName))
		{
			DEBUG_PRINTF("GenerateIcon Err \n");
			return -1;
		}
		//�����趨λ��
		dirIconX += DIR_FILE_ALL_WIDTH + ICON_MARGIN;
		//����Ҫ���� ��X �Ƿ񳬳�
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
	
	//���λ��Ϊ0˵��δ����λ��
	if(0 == atLayout[0].iTopLeftX && 0 == atLayout[0].iBottomRightX)
	{
		CalcBrowsePageLayout(ptPageLayout);
	}
	
	//��ȡ�Դ�
	ptVideoMem = GetVideoMem(ID("browse"), 1);
	if(! ptVideoMem)
	{
		DEBUG_PRINTF("cat't get video mem \n");
		return ;
	}
	
	//����
	if(GeneratePage(ptPageLayout, ptVideoMem) || GenerateBrowseDirIcon(ptVideoMem))
	{
		DEBUG_PRINTF("GeneratePage error \n");
		return ;
	}
	
	//ˢ��LCD
	//FlushVideoMemToDev(ptVideoMem);
	//��Ϊ�ػ�ҳ��������ػ���꣬����Ҫ����ʾ�����طŵ� �����Դ���
	FlushVideoMemToDevSync(ptVideoMem);

	//��ʾ�����ʷλ��
	ShowHistoryMouse();
	
	//�ͷ��Դ�
	PutVideoMem(ptVideoMem);
	
}

//��ʾ��һҳ
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

	//������һҳ����ʾ����
	g_iDirFilesIndex -= (g_iShowMaxFilesNum + g_iShowFilesNum);
	if(0 > g_iDirFilesIndex)
	{
		g_iDirFilesIndex = 0;
	}
	//����LCD����Ϣ
	GetDispResolution(&iXres, &iYres, &iBpp);
	//��ȡ�����Դ�
	ptVideoMem = GetVideoMem(ID("sync"), 1);
	//��Ϊ����ɫ
	CleanBrowseMainArea(ptVideoMem);

	//����layout �ڴ�
	if(! g_tBrowseFilesLayout.atLayout)
	{
		free(g_tBrowseFilesLayout.atLayout);
	}
	
	//���������ʾ���ٸ�ͼ��: ��һҳ�϶�������
	iNum = g_iShowMaxFilesNum;
	g_tBrowseFilesLayout.atLayout = malloc(sizeof(T_Layout) * iNum);
	if(! g_tBrowseFilesLayout.atLayout)
	{
		DEBUG_PRINTF("malloc g_tBrowseFilesLayout.atLayout error \n");
		return 0;
	}
	
	//���� ICON ��ʾ
	dirIconX   = ICON_MARGIN*2;
	dirIconY   = iYres * 2 / 10 + ICON_MARGIN;
	i = 0;
	while(i < iNum)
	{
		//����ͼ��Ҫ��ʾ��λ�ü��Դ�
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
		//����ͼ��
		if(GenerateIcon(ptVideoMem, &g_tBrowseFilesLayout.atLayout[i], ptDirFiles->strName))
		{
			DEBUG_PRINTF("GenerateIcon Err \n");
			return -1;
		}
		//�����趨λ��
		dirIconX += DIR_FILE_ALL_WIDTH + ICON_MARGIN;
		//����Ҫ���� ��X �Ƿ񳬳�
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

//��ʾ��һҳ
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

	//���������ʾ���ٸ�ͼ��: min(ÿҳ�����ʾ����, ʣ������)
	iNum = min(g_iShowMaxFilesNum, (g_iDirFilesNum - g_iDirFilesIndex));
	if(1 > iNum)
	{
		DEBUG_PRINTF("ShowBrowseNextPage last page\n");
		return -1;
	}
	
	//����LCD����Ϣ
	GetDispResolution(&iXres, &iYres, &iBpp);
	//��ȡ�����Դ�
	ptVideoMem = GetVideoMem(ID("sync"), 1);
	
	//��Ϊ����ɫ
	CleanBrowseMainArea(ptVideoMem);
	
	//����layout �ڴ�
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
	
	//���� ICON ��ʾ
	dirIconX   = ICON_MARGIN*2;
	dirIconY   = iYres * 2 / 10 + ICON_MARGIN;
	i = 0;
	while(i < iNum)
	{
		//����ͼ��Ҫ��ʾ��λ�ü��Դ�
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
		//����ͼ��
		if(GenerateIcon(ptVideoMem, &g_tBrowseFilesLayout.atLayout[i], ptDirFiles->strName))
		{
			DEBUG_PRINTF("GenerateIcon Err \n");
			return -1;
		}
		//�����趨λ��
		dirIconX += DIR_FILE_ALL_WIDTH + ICON_MARGIN;
		//����Ҫ���� ��X �Ƿ񳬳�
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
	int bPressed = 0;/* ���µ�״̬ */
	int iIndexPressed = 0; /* �����±� */
	char JumpDirPath[256];
	
	//��ʾ icons
	ShowBrowsePage(&g_tBrowsePageLayout);

	//�����¼�
	/* 3. ����GetInputEvent��������¼����������� */
	while (1)
	{
		iIndex = GenericGetInputEvent(&g_tBrowsePageLayout, &tInputEvent);
		//�����ɵ��ļ�ͼ���в���
		if(-1 == iIndex)
		{
			//�����������ͼռ�������� �ڴε����ʱ�ͷ���
			if(g_iUseMainArea)
			{
				if(g_iGIFPlay)
				{
					g_iGIFPlay = 0;
					GIFPlayStop();
				}
				//�ػ浱ǰ·��
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
		 *�жϰ����¼��㷨
		 *
		 */
		//�ɿ�״̬ ѹ��ֵ Ϊ 0
		if (0 == tInputEvent.iPressure)
		{
			if(bPressed)
			{
				bPressed = 0;
				//������º��ɿ���ͬһ������
				if(iIndexPressed == iIndex)
				{
					switch(iIndex)
					{
						case 0 : 
						{
							//�������·��
							snprintf(JumpDirPath, 256, "%s/%s", g_acDirPath, "..");
							realpath(JumpDirPath, g_acDirPath);
							BrowseDir();
						}
						break;

						case 1 : 
						{
							//ѡ��
						}
						break;

						case 2 :
						{
							//��һҳ
							ShowBrowsePrePage();
						}
						break;
						case 3 :
						{
							//��һҳ
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
			//�������ֵ��Ч
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


