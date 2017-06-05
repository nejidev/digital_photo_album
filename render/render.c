#include <render.h>
#include <conf.h>
#include <file.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pic_operation.h>
#include <page_manager.h>
#include <disp_manager.h>
#include <fonts_manager.h>
#include <encoding_manager.h>

int GetFontPixel()
{
	return 0;
}

int GetPicPixel()
{
	return 0;
}

int GetDrawPixel()
{
	return 0;
}

int MergePixelDatasToVideoMem(int x, int y, PT_PixelDatas ptPixelDatas, PT_VideoMem ptVideoMem)
{
	int i;
	unsigned char  *srcBuf;
	unsigned char  *dstBuf;
	srcBuf  = ptPixelDatas->aucPixelDatas;
	dstBuf  = ptVideoMem->tPixelDatas.aucPixelDatas;
	dstBuf += ptVideoMem->tPixelDatas.iLineBytes * y;
	dstBuf += x * ptVideoMem->tPixelDatas.iBpp / 8;
	
	for(i=y; i<(y + ptPixelDatas->iHeight); i++)
	{
		memcpy(dstBuf, srcBuf, ptPixelDatas->iLineBytes);
		srcBuf += ptPixelDatas->iLineBytes;
		dstBuf += ptVideoMem->tPixelDatas.iLineBytes;
	}
	return 0;
}


int MergeFontBitmapToPixelDatas(PT_FontBitMap ptFontBitMap, PT_PixelDatas ptPixelDatas)
{
	//字模
 	unsigned char *buf;
	int i;
	int x;
	int y;

	for(y = ptFontBitMap->iYTop; y<ptFontBitMap->iYMax; y++)
	{
		//对于freetype buffer 的开始位置不一定是 1 可能一行需要2－3个来显示
		buf = (y - ptFontBitMap->iYTop) * ptFontBitMap->iPitch + ptFontBitMap->pucBuffer;
		for(x = ptFontBitMap->iXLeft; x<ptFontBitMap->iXMax; x+=8)
		{
			for(i=0; i<8; i++)
			{
				if(*buf & (1<<(7-i)))
				{
					ShowPixelPixelDatasMem(ptPixelDatas, x+i, y, COLOR_FOREGROUND);
				}
				else
				{
					ShowPixelPixelDatasMem(ptPixelDatas, x+i, y, COLOR_BACKGROUND);
				}	
			}
			buf++;
		}
	}
	//计算下一个原点
	ptFontBitMap->iCurOriginX = ptFontBitMap->iNextOriginX;
	ptFontBitMap->iCurOriginY = ptFontBitMap->iNextOriginY;
	return 0;
}

int GetPixelDatasForFreetype(unsigned char *str, PT_PixelDatas ptPixelDatas)
{
	int iError;
	PT_FontOpr   ptFontOpr;
	T_FontBitMap tFontBitMap;
	int iLen = 0;
	unsigned char *pucBufStart;
	unsigned char *pucBufEnd;
	unsigned int dwCode;
	int fontWitdh  = 0;
	int fontHeight = 0;
	int fontNum = 0; //可以显示的字符个数

	int iMinX = 32000, iMaxX = -1;
	int iMinY = 32000, iMaxY = -1;
	pucBufStart = str;
	pucBufEnd   = str + strlen((char *)str); 
	ptFontOpr    = GetFontOpr("freetype");
	//ptFontOpr = GetFontOpr("ascii");
	
	//设置原点
	tFontBitMap.iCurOriginX = 0;
	tFontBitMap.iCurOriginY = 0;
	
	//获取最多能显示多少个字符
	while (1)
	{
		iLen = GetCodeFrmBuf(pucBufStart, pucBufEnd, &dwCode);
		/* 字符串结束 */
		if(0 == iLen)
		{
			break;
		}
		pucBufStart += iLen;
		/* 获得字符的位图, 位图信息里含有字符显示时的左上角、右下角坐标 */
		iError = ptFontOpr->GetFontBitMap(dwCode, &tFontBitMap);
		if(iError)
		{
			DEBUG_PRINTF("GetPixelDatasForFreetype error \n");
			return -1;
		}
		if (iMinX > tFontBitMap.iXLeft)
		{
			iMinX = tFontBitMap.iXLeft;
		}
		if (iMaxX < tFontBitMap.iXMax)
		{
			iMaxX = tFontBitMap.iXMax;
		}
		if (iMinY > tFontBitMap.iYTop)
		{
			iMinY = tFontBitMap.iYTop;
		}
		if (iMaxY < tFontBitMap.iXMax)
		{
			iMaxY = tFontBitMap.iYMax;
		}
			
		tFontBitMap.iCurOriginX = tFontBitMap.iNextOriginX;
		tFontBitMap.iCurOriginY = tFontBitMap.iNextOriginY;

		//计算字符的大小
		fontWitdh  = iMaxX - iMinX;
		fontHeight = iMaxY - iMinY;
		//如果字符的宽度大于 容器的宽度就退出
		if(fontWitdh > ptPixelDatas->iWidth)
		{
			fontNum--;
			break;
		}
		if(fontHeight > ptPixelDatas->iHeight)
		{
			DEBUG_PRINTF("GetPixelDatasForFreetype err Height allow \n");
			return -1;
		}
		//有效字符个数++
		fontNum++;
	}

	pucBufStart = str;
	pucBufEnd   = str + strlen((char *)str); 

	//设置原点 在中间居中显示
	tFontBitMap.iCurOriginX = (ptPixelDatas->iWidth  - fontWitdh)  / 2;
	tFontBitMap.iCurOriginY = (ptPixelDatas->iHeight - fontHeight) / 2 + fontHeight;
	
	//生成在画布上
	while(fontNum)
	{
		iLen = GetCodeFrmBuf(pucBufStart, pucBufEnd, &dwCode);
		/* 字符串结束 */
		if(0 == iLen)
		{
			break;
		}
		pucBufStart += iLen;
		/* 获得字符的位图, 位图信息里含有字符显示时的左上角、右下角坐标 */
		iError = ptFontOpr->GetFontBitMap(dwCode, &tFontBitMap);
		if(iError)
		{
			DEBUG_PRINTF("GetPixelDatasForFreetype error \n");
			return -1;
		}
		//生成
		MergeFontBitmapToPixelDatas(&tFontBitMap, ptPixelDatas);
		fontNum--;
	}
	return 0;
}

int GetPixelDatasForIcon(char *strFileName, PT_PixelDatas ptPixelDatas)
{
	T_FileMap tFileMap;
	PT_PicFileParser ptPicFileParser;

	//打开文件进行 mmap
	strncpy(tFileMap.strFileName, strFileName, 256);
	tFileMap.strFileName[255] = '\0';
	if(MapFile(&tFileMap))
	{
		DEBUG_PRINTF("cat't GetPixelDatasForIcon error \n");
		return -1;
	}
	
	//查找合适的解析器
	ptPicFileParser = MatchParser(tFileMap.pucFileMapMem);
	//ptPicFileParser = Parser("bmp");
	
	//解析图片放到 ptPixelDatas
	if(ptPicFileParser->GetPixelDatas(tFileMap.pucFileMapMem, ptPixelDatas))
	{
		UnMapFile(&tFileMap);
		DEBUG_PRINTF("cat't ptPicFileParser GetPixelDatas error \n");
		return -1;
	}
	UnMapFile(&tFileMap);
	return 0;
}

//将指定显示区域的颜色取反
static void InvertButton(PT_Layout ptLayout)
{
	int i;
	int y;
	unsigned char  *videoMem;
	int             iButtonWidthBytes;
	PT_DispOpr      ptDispOpr;
	PT_VideoMem     ptSyncVideoMem;
	ptDispOpr       = GetDefaultDispDev();
	ptSyncVideoMem  = GetVideoMem(ID("sync"), 1);
	
	//绘制到镜像显存上
	videoMem  = ptSyncVideoMem->tPixelDatas.aucPixelDatas + ptLayout->iTopLeftY * ptDispOpr->iLineWidth;
	videoMem += ptLayout->iTopLeftX * ptDispOpr->iBpp / 8;
	iButtonWidthBytes = (ptLayout->iBottomRightX - ptLayout->iTopLeftX) * ptDispOpr->iBpp / 8;
	//将图片像素取反
	for(y = ptLayout->iTopLeftY; y< ptLayout->iBottomRightY; y++)
	{
		for(i = 0; i< iButtonWidthBytes; i++)
		{
			videoMem[i] = ~videoMem[i];
		}
		videoMem += ptDispOpr->iLineWidth;
	}
	//同步镜像显存到LCD
	memcpy(ptDispOpr->pucDispMem, ptSyncVideoMem->tPixelDatas.aucPixelDatas, ptSyncVideoMem->tPixelDatas.iTotalBytes);

	//重绘鼠标
	ShowHistoryMouse();
	
}

void PressButton(PT_Layout ptLayout)
{
	InvertButton(ptLayout);
}

void ReleaseButton(PT_Layout ptLayout)
{
	InvertButton(ptLayout);
}
