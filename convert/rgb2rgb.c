#include <convert_manager.h>
#include <stdlib.h>
#include <string.h>

static int Rgb2rgbIsSupport(int iPixelFormatIn, int iPixelFormatOut)
{
	if(V4L2_PIX_FMT_RGB565 != iPixelFormatOut)
	{
		return 0;
	}

	if(V4L2_PIX_FMT_RGB565 != iPixelFormatIn)
	{
		return 0;
	}
	return 1;
}

static int Rgb2rgbConvert(PT_VideoBuf ptVideoBufIn, PT_VideoBuf ptVideoBufOut)
{
	//读取数据转为 GBK 
	PT_PixelDatas ptPixelDatasIn  = &ptVideoBufIn->tPixelDatas;
    PT_PixelDatas ptPixelDatasOut = &ptVideoBufOut->tPixelDatas;

    ptPixelDatasOut->iWidth  = ptPixelDatasIn->iWidth;
    ptPixelDatasOut->iHeight = ptPixelDatasIn->iHeight;
    
    if (ptVideoBufOut->iPixelFormat == V4L2_PIX_FMT_RGB565)
    {
		ptPixelDatasOut->iBpp = 16;
        ptPixelDatasOut->iLineBytes  = ptPixelDatasOut->iWidth * ptPixelDatasOut->iBpp / 8;
        ptPixelDatasOut->iTotalBytes = ptPixelDatasOut->iLineBytes * ptPixelDatasOut->iHeight;

        if (! ptPixelDatasOut->aucPixelDatas)
        {
            ptPixelDatasOut->aucPixelDatas = malloc(ptPixelDatasOut->iTotalBytes);
        }
		memcpy(ptPixelDatasOut->aucPixelDatas, ptPixelDatasIn->aucPixelDatas, ptPixelDatasOut->iTotalBytes);
        return 0;
	}
 	return -1;
}

static int Rgb2rgbConvertExit(PT_VideoBuf ptVideoBufOut)
{
	if (! ptVideoBufOut->tPixelDatas.aucPixelDatas)
    {
     	free(ptVideoBufOut->tPixelDatas.aucPixelDatas);
		ptVideoBufOut->tPixelDatas.aucPixelDatas = NULL;
	}
	return 0;
}

static T_VideoConvert tRgb2rgbVideoConvert = {
	.name        = "rgb",
	.isSupport   = Rgb2rgbIsSupport,
	.Convert     = Rgb2rgbConvert,
	.ConvertExit = Rgb2rgbConvertExit,
};

int InitRgb2rgb(void)
{
    return RegisterVideoConvert(&tRgb2rgbVideoConvert);
}

