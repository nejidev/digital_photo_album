#include <convert_manager.h>

static PT_VideoConvert g_ptVideoConvertHead;

int RegisterVideoConvert(PT_VideoConvert ptVideoConvert)
{
	PT_VideoConvert temp;
	if(! g_ptVideoConvertHead)
	{
		g_ptVideoConvertHead = ptVideoConvert;
	}
	else
	{
		temp = g_ptVideoConvertHead;
		while(temp->ptNext)
		{
			temp = temp->ptNext;
		}
		temp->ptNext = ptVideoConvert;
	}
	ptVideoConvert->ptNext = NULL;
	return 0;
}

PT_VideoConvert GetVideoConvertForFormats(int iPixelFormatIn, int iPixelFormatOut)
{
	PT_VideoConvert ptTmp = g_ptVideoConvertHead;
	//printf("V4L2_PIX_FMT_RGB565 : 0x%x \n", V4L2_PIX_FMT_RGB565);
	//printf("V4L2_PIX_FMT_YUYV : 0x%x \n", V4L2_PIX_FMT_YUYV);
	//printf("V4L2_PIX_FMT_MJPEG : 0x%x \n", V4L2_PIX_FMT_MJPEG);
	while (ptTmp)
	{
        if (ptTmp->isSupport(iPixelFormatIn, iPixelFormatOut))
        {
            return ptTmp;
        }
		ptTmp = ptTmp->ptNext;
	}
	return NULL;
}


int InitVideoConvert(void)
{
	int iError = 0;
	iError |= InitMjpeg2rgb();
	iError |= InitYuv2rgb();
	iError |= InitRgb2rgb();
	return iError;
}
