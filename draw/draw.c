#include <conf.h>
#include <draw.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <encoding_manager.h>
#include <fonts_manager.h>
#include <disp_manager.h>
#include <string.h>

//打开小说 fd
static int g_iFdTextFile;
//小说mmap开始地址
static unsigned char *g_pucTextFileMem;
//小说mmap结束地址
static unsigned char *g_pucTextFileMemEnd;
//文件解码器
static PT_EncodingOpr g_ptEncodingOprForFile;
//小说真实编码开始地址
static unsigned char *g_pucLcdFirstPosAtFile;
//分页链表
static PT_PageDesc g_ptPages = NULL;
static PT_PageDesc g_ptCurPage = NULL;
//lcd显示
static PT_DispOpr g_ptDispOpr;
//字体大小
static int g_dwFontSize;
//当前页结束的编码地址
static unsigned char *g_pucLcdNextPosAtFile;


int SetFontsDetail(char *pcHZKFile, char *pcFileFreetype, unsigned dwFontSize)
{
	int iError = 0;
	//初始化font
	PT_FontOpr ptFontOpr = g_ptEncodingOprForFile->ptFontOprSupportedHead;
	PT_FontOpr pttmpFontOpr; 

	while(ptFontOpr)
	{
		if(0 == strcmp(ptFontOpr->name, "ascii"))
		{
			iError = ptFontOpr->FontInit(NULL, dwFontSize);
		}
		else if(0 == strcmp(ptFontOpr->name, "gbk"))
		{
			iError = ptFontOpr->FontInit(pcHZKFile, dwFontSize);
		}
		else if(0 == strcmp(ptFontOpr->name, "freetype"))
		{
			iError = ptFontOpr->FontInit(pcFileFreetype, dwFontSize);
		}
		//防治 free 以后找不到 下一个
		pttmpFontOpr = ptFontOpr->ptNext;
		if(iError)
		{
			DelFontOprFrmEncoding(g_ptEncodingOprForFile, ptFontOpr);
		}
		ptFontOpr = pttmpFontOpr;
	}
	g_dwFontSize = dwFontSize;
	return 0;
}

static void DrawFontBitMap(PT_FontBitMap pt_FontBitMap)
{
	//字模
 	unsigned char *buf;
	int i;
	int x;
	int y;
	
	for(y = pt_FontBitMap->iYTop; y<pt_FontBitMap->iYMax; y++)
	{
		//对于freetype buffer 的开始位置不一定是 1 可能一行需要2－3个来显示
		buf = (y - pt_FontBitMap->iYTop) * pt_FontBitMap->iPitch + pt_FontBitMap->pucBuffer;
		for(x = pt_FontBitMap->iXLeft; x<pt_FontBitMap->iXMax; x+=8)
		{
			for(i=0; i<8; i++)
			{
				if(*buf & (1<<(7-i)))
				{
					g_ptDispOpr->ShowPixel(x+i, y, COLOR_FOREGROUND);
				}
			}
			buf++;
		}
	}
}

static int IncLcdY(int iY)
{
	if (iY + g_dwFontSize < g_ptDispOpr->iYres)
		return (iY + g_dwFontSize);
	else
		return 0;
}

static int RelocateFontPos(PT_FontBitMap ptFontBitMap)
{
	int iLcdY;
	int iDeltaX;
	int iDeltaY;
	
	if (ptFontBitMap->iYMax > g_ptDispOpr->iYres)
	{
		/* 满页了 */
		return -1;
	}
	/* 如果右边显示不下就换行 */
	if(ptFontBitMap->iXMax > g_ptDispOpr->iXres)
	{
		iLcdY = IncLcdY(ptFontBitMap->iCurOriginY);
		/* 满页了 */
		if (0 == iLcdY)
		{
			return -1;
		}
		/* 没满页 */
		iDeltaX = 0 - ptFontBitMap->iCurOriginX;
		iDeltaY = iLcdY - ptFontBitMap->iCurOriginY;

		ptFontBitMap->iCurOriginX  += iDeltaX;
		ptFontBitMap->iCurOriginY  += iDeltaY;

		ptFontBitMap->iNextOriginX += iDeltaX;
		ptFontBitMap->iNextOriginY += iDeltaY;

		ptFontBitMap->iXLeft += iDeltaX;
		ptFontBitMap->iXMax  += iDeltaX;

		ptFontBitMap->iYTop  += iDeltaY;
		ptFontBitMap->iYMax  += iDeltaY;;
	}
	return 0;
}

static int ShowPage(unsigned char *pucTextFileMemCurPos)
{
	int iError;
	unsigned char *pucBuffer = pucTextFileMemCurPos;
	unsigned int textCode;
	int len; 
 	PT_FontOpr ptFontOpr;
	T_FontBitMap tFontBitMap;
	//设置初始化时绘开始位置
	tFontBitMap.iCurOriginX = 0;
	tFontBitMap.iCurOriginY = g_dwFontSize;
	/* 首先清屏 */
	g_ptDispOpr->CleanScreen(COLOR_BACKGROUND);
	//绘制整个屏幕
	while(1)
	{
		len = g_ptEncodingOprForFile->GetCodeFrmBuf(pucBuffer, g_pucTextFileMemEnd, &textCode);
		if(0 == len)
		{
			DEBUG_PRINTF("read file end \n");
			break;
		}
		pucBuffer += len;
		
		//处理换行
		if('\n' == textCode)
		{
			tFontBitMap.iCurOriginX = 0;
			tFontBitMap.iCurOriginY = IncLcdY(tFontBitMap.iCurOriginY);
			if (0 == tFontBitMap.iCurOriginY)
			{
				/* 显示完当前一屏了 */
				g_pucLcdNextPosAtFile = pucBuffer;
				return 0;
			}
			else
			{
				continue;
			};
		}
		//\r不处理
		if('\r' == textCode)
		{
			continue;
		}
		//tab空格3个
		if('\t' == textCode)
		{
			textCode = ' ';
		}
		ptFontOpr = g_ptEncodingOprForFile->ptFontOprSupportedHead;
		while(ptFontOpr)
		{
			iError = ptFontOpr->GetFontBitMap(textCode, &tFontBitMap);
			//如果获取到 bitmap 成功 就返回
			if(0 == iError)
			{
				if (RelocateFontPos(&tFontBitMap))
				{
					/* 剩下的LCD空间不能满足显示这个字符 */
					return 0;
				}
				DrawFontBitMap(&tFontBitMap);
				//计算下一个原点
				tFontBitMap.iCurOriginX = tFontBitMap.iNextOriginX;
				tFontBitMap.iCurOriginY = tFontBitMap.iNextOriginY;
				//每次记录结束地址 当自动退出时 就保存 了最后一个位置
				g_pucLcdNextPosAtFile = pucBuffer;
				break;
			}
			//如果不支持就调用下一个
			ptFontOpr = ptFontOpr->ptNext;
		}
	}
	return 0;
}

void RecordPage(PT_PageDesc pt_PageDescNew)
{
	if(! g_ptPages)
	{
		g_ptPages = pt_PageDescNew;
		g_ptPages->ptNextPage = NULL;
		g_ptPages->ptPrePage  = NULL;
	}
	else
	{
		while(g_ptPages->ptNextPage)
		{
			g_ptPages = g_ptPages->ptNextPage;
		}
		pt_PageDescNew->ptPrePage = g_ptPages;
		g_ptPages->ptNextPage = pt_PageDescNew;
	}
}

int ShowFirstPage(void)
{
	return ShowNextPage();
}

int ShowNextPage(void)
{
	int iError;
	PT_PageDesc ptPage;
	unsigned char *pucTextFileMemCurPos;

	if (g_ptCurPage)
	{
		pucTextFileMemCurPos = g_ptCurPage->pucLcdNextPageFirstPosAtFile;
	}
	else
	{
		pucTextFileMemCurPos = g_pucLcdFirstPosAtFile;
	}
	ShowPage(pucTextFileMemCurPos);

	//如果有下了页就不需要在创建了
	if(g_ptPages && g_ptCurPage->ptNextPage)
	{
		g_ptCurPage = g_ptCurPage->ptNextPage;
		return 0;
	}
	
	ptPage = malloc(sizeof(T_PageDesc));
	if(!ptPage)
	{
		DEBUG_PRINTF("malloc pagedesc error \n");
		iError = -1;
	}
	else
	{
		ptPage->pucLcdFirstPosAtFile = pucTextFileMemCurPos;
		ptPage->pucLcdNextPageFirstPosAtFile = g_pucLcdNextPosAtFile;
		ptPage->ptNextPage = NULL;
		ptPage->ptPrePage  = NULL;
		RecordPage(ptPage);
		g_ptCurPage = ptPage;
	}
	return iError;
}

int ShowPrePage(void)
{
	int iError;
	if(! g_ptCurPage || ! g_ptCurPage->ptPrePage)
	{
		iError = -1;
	}
	else
	{
		g_ptCurPage = g_ptCurPage->ptPrePage;
		ShowPage(g_ptCurPage->pucLcdFirstPosAtFile);
		iError = 0;
	}
	return iError;
}

int SelectAndInitDisplay(char *pcName)
{
	g_ptDispOpr = GetDispOpr(pcName);
	if(NULL == g_ptDispOpr)
	{
		return -1;
	}
	g_ptDispOpr->CleanScreen(COLOR_BACKGROUND);
	return 0;
}

int OpenTextFile(char * pcFileName)
{
	struct stat tStat;
	g_iFdTextFile = open(pcFileName, O_RDONLY);
	if(0 > g_iFdTextFile)
	{
		DEBUG_PRINTF("cat't open %s \n", pcFileName);
		return -1;
	}
	//获取文件大小
	if(fstat(g_iFdTextFile, &tStat))
	{
		DEBUG_PRINTF("cat't get fstat \n");
		return -1;
	}
	//mmap
	g_pucTextFileMem = (unsigned char *)mmap(NULL, tStat.st_size, PROT_READ, MAP_SHARED, g_iFdTextFile, 0);
	if(MAP_FAILED == g_pucLcdFirstPosAtFile)
	{
		DEBUG_PRINTF("cat't file mmap \n");
		return -1;
	}
	//设置文件结束
	g_pucTextFileMemEnd = g_pucTextFileMem + tStat.st_size;

	g_ptEncodingOprForFile = SelectEncodingOprForFile(g_pucTextFileMem);
	if(g_ptEncodingOprForFile)
	{
		//移动指针跳转头部
		g_pucLcdFirstPosAtFile = g_pucTextFileMem + g_ptEncodingOprForFile->iHeadLen;
		return 0;
	}
	else
	{
		DEBUG_PRINTF("cat't SelectEncodingOprForFile \n");
		return -1;
	} 
	return 0;
}
