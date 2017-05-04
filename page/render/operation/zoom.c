
#include <pic_operation.h>
#include <stdlib.h>
#include <string.h>

/**
 * 图片缩放代码
 * 最简单的实现，临近取值法 算法简单，缺点放大后会有锯齿
 */
int PicZoom(PT_PixelDatas ptOriginPic, PT_PixelDatas ptZoomPic)
{
    unsigned long dwDstWidth = ptZoomPic->iWidth;
	//先计算出一行的取样点位置
    unsigned long *pdwSrcXTable = malloc(sizeof(unsigned long) *dwDstWidth);
	unsigned long x;
	unsigned long y;
	unsigned char *pucDest;
	unsigned char *pucSrc;
	unsigned long dwPixelBytes = ptOriginPic->iBpp/8;

	if (ptOriginPic->iBpp != ptZoomPic->iBpp)
	{
		return -1;
	}
	//计算取样表
	for(x=0; x<dwDstWidth; x++)
	{
		pdwSrcXTable[x] = x * ptOriginPic->iWidth / ptZoomPic->iWidth;
	}
	//生成缩放图
	for(y=0; y<ptZoomPic->iHeight; y++)
	{
		pucDest = ptZoomPic->aucPixelDatas + y * ptZoomPic->iLineBytes;
		pucSrc  = ptOriginPic->aucPixelDatas + y * ptOriginPic->iHeight / ptZoomPic->iHeight * ptOriginPic->iLineBytes;
		for(x=0; x<dwDstWidth; x++)
		{
			/* 原图座标: pdwSrcXTable[x]，srcy             
			* 缩放座标: x, y			 */
			memcpy(pucDest+x*dwPixelBytes, pucSrc+pdwSrcXTable[x]*dwPixelBytes, dwPixelBytes);
		}
	}
	
	free(pdwSrcXTable);
	return 0;
}

