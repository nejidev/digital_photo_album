
#include <pic_operation.h>
#include <string.h>

//将小图合并到大图的指定位置
int PicMerge(int iX, int iY, PT_PixelDatas ptSmallPic, PT_PixelDatas ptBigPic)
{
	int i;
	unsigned char *pucSrc;
	unsigned char *pucDst;

	//判断大小是否超过 BPP 是否相同
	if((ptSmallPic->iBpp != ptBigPic->iBpp) || ((ptSmallPic->iHeight + iY) > ptBigPic->iHeight) 
		|| ((ptSmallPic->iWidth + iX) > ptBigPic->iWidth))
	{
		return -1;
	}
	//合并文件
	#if 1
	pucSrc = ptSmallPic->aucPixelDatas;
	pucDst = ptBigPic->aucPixelDatas + iY*ptBigPic->iLineBytes + iX*ptBigPic->iBpp/8;
	for(i=0; i<ptSmallPic->iHeight; i++)
	{
		memcpy(pucDst, pucSrc, ptSmallPic->iLineBytes);
		pucSrc += ptSmallPic->iLineBytes;
		pucDst += ptBigPic->iLineBytes; 
	}
	#else
	//我的算法效率不高
	pucSrc = ptSmallPic->aucPixelDatas;
	pucDst = ptBigPic->aucPixelDatas;
	for(i=iY; i<(iY + ptSmallPic->iHeight); i++)
	{
		pucSrc += ptSmallPic->iLineBytes; 
		pucDst += ptBigPic->iLineBytes + iX*ptBigPic->iBpp/8;;
		memcpy(pucDst, pucSrc, ptSmallPic->iLineBytes);
	}
	#endif
	return 0;
}

