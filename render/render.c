#include <render.h>
#include <conf.h>
#include <file.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pic_operation.h>
#include <page_manager.h>
#include <disp_manager.h>

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

int GetPixelDatasForIcon(char *strFileName, PT_PixelDatas ptPixelDatas)
{
	T_FileMap tFileMap;
	PT_PicFileParser ptPicFileParser;

	//打开文件进行 mmap
	snprintf(tFileMap.strFileName, 256, "%s/%s", ICON_DIR, strFileName);
	
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