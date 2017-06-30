#include <convert_manager.h>
#include <stdlib.h>
#include "color.h"

static int Yuv2rgbIsSupport(int iPixelFormatIn, int iPixelFormatOut)
{
	if(V4L2_PIX_FMT_RGB565 != iPixelFormatOut)
	{
		return 0;
	}

	if(V4L2_PIX_FMT_YUYV != iPixelFormatIn)
	{
		return 0;
	}
	return 1;
}

/* translate YUV422Packed to rgb24 */
static unsigned int
Pyuv422torgb565(unsigned char * input_ptr, unsigned char * output_ptr, unsigned int image_width, unsigned int image_height)
{
	unsigned int i, size;
	unsigned char Y, Y1, U, V;
	unsigned char *buff = input_ptr;
	unsigned char *output_pt = output_ptr;

    unsigned int r, g, b;
    unsigned int color;
    
	size = image_width * image_height /2;
	for (i = size; i > 0; i--) {
		/* bgr instead rgb ?? */
		Y = buff[0] ;
		U = buff[1] ;
		Y1 = buff[2];
		V = buff[3];
		buff += 4;
		r = R_FROMYV(Y,V);
		g = G_FROMYUV(Y,U,V); //b
		b = B_FROMYU(Y,U); //v

        /* 把r,g,b三色构造为rgb565的16位值 */
        r = r >> 3;
        g = g >> 2;
        b = b >> 3;
        color = (r << 11) | (g << 5) | b;
        *output_pt++ = color & 0xff;
        *output_pt++ = (color >> 8) & 0xff;
			
		r = R_FROMYV(Y1,V);
		g = G_FROMYUV(Y1,U,V); //b
		b = B_FROMYU(Y1,U); //v
		
        /* 把r,g,b三色构造为rgb565的16位值 */
        r = r >> 3;
        g = g >> 2;
        b = b >> 3;
        color = (r << 11) | (g << 5) | b;
        *output_pt++ = color & 0xff;
        *output_pt++ = (color >> 8) & 0xff;
	}
	
	return 0;
}

static int Yuv2rgbConvert(PT_VideoBuf ptVideoBufIn, PT_VideoBuf ptVideoBufOut)
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
        Pyuv422torgb565(ptPixelDatasIn->aucPixelDatas, ptPixelDatasOut->aucPixelDatas, ptPixelDatasOut->iWidth, ptPixelDatasOut->iHeight);
		return 0;
	}
	return -1;
}

static int Yuv2rgbConvertExit(PT_VideoBuf ptVideoBufOut)
{
	 if (! ptVideoBufOut->tPixelDatas.aucPixelDatas)
     {
     	free(ptVideoBufOut->tPixelDatas.aucPixelDatas);
		ptVideoBufOut->tPixelDatas.aucPixelDatas = NULL;
	 }
	return 0;
}

static T_VideoConvert g_tYuv2RgbConvert = {
	.name        = "yuv",
	.isSupport   = Yuv2rgbIsSupport,
	.Convert     = Yuv2rgbConvert,
	.ConvertExit = Yuv2rgbConvertExit,
};

extern void initLut(void);

int InitYuv2rgb(void)
{
	initLut();
    return RegisterVideoConvert(&g_tYuv2RgbConvert);
}
