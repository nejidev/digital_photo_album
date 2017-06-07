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
	//��ģ
 	unsigned char *buf;
	int i;
	int x;
	int y;

	for(y = ptFontBitMap->iYTop; y<ptFontBitMap->iYMax; y++)
	{
		//����freetype buffer �Ŀ�ʼλ�ò�һ���� 1 ����һ����Ҫ2��3������ʾ
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
	//������һ��ԭ��
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
	int fontNum = 0; //������ʾ���ַ�����

	int iMinX = 32000, iMaxX = -1;
	int iMinY = 32000, iMaxY = -1;
	pucBufStart = str;
	pucBufEnd   = str + strlen((char *)str); 
	ptFontOpr    = GetFontOpr("freetype");
	//ptFontOpr = GetFontOpr("ascii");
	
	//����ԭ��
	tFontBitMap.iCurOriginX = 0;
	tFontBitMap.iCurOriginY = 0;
	
	//��ȡ�������ʾ���ٸ��ַ�
	while (1)
	{
		iLen = GetCodeFrmBuf(pucBufStart, pucBufEnd, &dwCode);
		/* �ַ������� */
		if(0 == iLen)
		{
			break;
		}
		pucBufStart += iLen;
		/* ����ַ���λͼ, λͼ��Ϣ�ﺬ���ַ���ʾʱ�����Ͻǡ����½����� */
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

		//�����ַ��Ĵ�С
		fontWitdh  = iMaxX - iMinX;
		fontHeight = iMaxY - iMinY;
		//����ַ��Ŀ�ȴ��� �����Ŀ�Ⱦ��˳�
		if(fontWitdh > ptPixelDatas->iWidth)
		{
			fontNum -= 1;
			break;
		}
		if(fontHeight > ptPixelDatas->iHeight)
		{
			DEBUG_PRINTF("GetPixelDatasForFreetype err Height allow \n");
			return -1;
		}
		//��Ч�ַ�����++
		fontNum++;
	}

	pucBufStart = str;
	pucBufEnd   = str + strlen((char *)str); 

	//����ԭ�� ���м������ʾ
	tFontBitMap.iCurOriginX =0;// (ptPixelDatas->iWidth  - fontWitdh)  / 2; �Զ�������ʾ Ҳ�е����� ����������
	tFontBitMap.iCurOriginY = (ptPixelDatas->iHeight - fontHeight) / 2 + fontHeight;

	//�Զ�������ʾ�������е����⣬���ֹ�ָ��
	fontNum = 8;
	
	//�����ڻ�����
	while(fontNum)
	{
		iLen = GetCodeFrmBuf(pucBufStart, pucBufEnd, &dwCode);
		/* �ַ������� */
		if(0 == iLen)
		{
			break;
		}
		pucBufStart += iLen;
		/* ����ַ���λͼ, λͼ��Ϣ�ﺬ���ַ���ʾʱ�����Ͻǡ����½����� */
		iError = ptFontOpr->GetFontBitMap(dwCode, &tFontBitMap);
		if(iError)
		{
			DEBUG_PRINTF("GetPixelDatasForFreetype error \n");
			return -1;
		}
		//����
		MergeFontBitmapToPixelDatas(&tFontBitMap, ptPixelDatas);
		fontNum--;
	}
	return 0;
}

int GetPixelDatasForIcon(char *strFileName, PT_PixelDatas ptPixelDatas)
{
	T_FileMap tFileMap;
	PT_PicFileParser ptPicFileParser;

	//���ļ����� mmap
	strncpy(tFileMap.strFileName, strFileName, 256);
	tFileMap.strFileName[255] = '\0';
	if(MapFile(&tFileMap))
	{
		DEBUG_PRINTF("cat't GetPixelDatasForIcon error \n");
		return -1;
	}
	
	//���Һ��ʵĽ�����
	ptPicFileParser = MatchParser(tFileMap.pucFileMapMem, tFileMap.iFileSize);
	//ptPicFileParser = Parser("bmp");
	
	//����ͼƬ�ŵ� ptPixelDatas
	if(ptPicFileParser->GetPixelDatas(tFileMap.pucFileMapMem, ptPixelDatas))
	{
		UnMapFile(&tFileMap);
		DEBUG_PRINTF("cat't ptPicFileParser GetPixelDatas error \n");
		return -1;
	}
	UnMapFile(&tFileMap);
	return 0;
}

//��ָ����ʾ�������ɫȡ��
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
	
	//���Ƶ������Դ���
	videoMem  = ptSyncVideoMem->tPixelDatas.aucPixelDatas + ptLayout->iTopLeftY * ptDispOpr->iLineWidth;
	videoMem += ptLayout->iTopLeftX * ptDispOpr->iBpp / 8;
	iButtonWidthBytes = (ptLayout->iBottomRightX - ptLayout->iTopLeftX) * ptDispOpr->iBpp / 8;
	//��ͼƬ����ȡ��
	for(y = ptLayout->iTopLeftY; y< ptLayout->iBottomRightY; y++)
	{
		for(i = 0; i< iButtonWidthBytes; i++)
		{
			videoMem[i] = ~videoMem[i];
		}
		videoMem += ptDispOpr->iLineWidth;
	}
	//ͬ�������Դ浽LCD
	memcpy(ptDispOpr->pucDispMem, ptSyncVideoMem->tPixelDatas.aucPixelDatas, ptSyncVideoMem->tPixelDatas.iTotalBytes);

	//�ػ����
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
