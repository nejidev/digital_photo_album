#ifndef __RENDER_H
#define __RENDER_H
#include <pic_operation.h>
#include <disp_manager.h>
#include <fonts_manager.h>

int PicZoom(PT_PixelDatas ptOriginPic, PT_PixelDatas ptZoomPic);
int PicMerge(int iX, int iY, PT_PixelDatas ptSmallPic, PT_PixelDatas ptBigPic);
int GetPixelDatasForFreetype(char *str, PT_PixelDatas ptPixelDatas);
int GetPixelDatasForIcon(char *strFileName, PT_PixelDatas ptPixelDatas);
void PressButton(PT_Layout ptLayout);
void ReleaseButton(PT_Layout ptLayout);
int MergePixelDatasToVideoMem(int x, int y, PT_PixelDatas ptPixelDatas, PT_VideoMem ptVideoMem);
int MergeFontBitmapToPixelDatas(PT_FontBitMap ptFontBitMap, PT_PixelDatas ptPixelDatas);

#endif

