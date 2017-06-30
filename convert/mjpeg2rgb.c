#include <convert_manager.h>
#include <pic_operation.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <setjmp.h>

static int Mjpeg2rgbIsSupport(int iPixelFormatIn, int iPixelFormatOut)
{
	if(V4L2_PIX_FMT_RGB565 != iPixelFormatOut)
	{
		return 0;
	}

	if(V4L2_PIX_FMT_MJPEG != iPixelFormatIn)
	{
		return 0;
	}
	return 1;
}

/*
1. Allocate and initialize a JPEG decompression object.
2. Specify the source of the compressed data (eg, a file).
3. Call jpeg_read_header() to obtain image info.
4. Set parameters for decompression.
5. jpeg_start_decompress(...);
6. while (scan lines remain to be read)
	jpeg_read_scanlines(...);
7. jpeg_finish_decompress(...);
8. Release the JPEG decompression object
*/
static struct jpeg_decompress_struct cinfo;
static struct jpeg_error_mgr jerr;

static int ConvertOneLine(int iWidth, int iBMPBpp, int iDstBpp, unsigned char *pucSrc, unsigned char *pucDest)
{
	unsigned int dwRed;
	unsigned int dwGreen;
	unsigned int dwBlue;
	unsigned short *pucDest16bpp = (unsigned short *)pucDest;
	unsigned int   *pucDest32bpp = (unsigned int *)pucDest;
	int pos = 0;
	int i;
	
	if(24 != iBMPBpp)
	{
		return -1;
	}
	if(24 == iDstBpp)
	{
		memcpy(pucDest, pucSrc, iWidth*3);
	}
	else
	{
		for(i=0; i<iWidth; i++)
		{
			//JPG ����ɫ������ RRGGBB
			dwRed   = pucSrc[pos++];
			dwGreen = pucSrc[pos++];
			dwBlue  = pucSrc[pos++];
			if(32 == iDstBpp)
			{
				*pucDest32bpp = (dwRed<<16) | (dwGreen<<8) | dwBlue;
				pucDest32bpp++;
			}
			if(16 == iDstBpp)
			{
				//565
				dwRed   = dwRed>>3;
				dwGreen = dwGreen>>2;
				dwBlue  = dwBlue>>3;
				*pucDest16bpp = (dwRed<<11) | (dwGreen<<5) | dwBlue;
				pucDest16bpp++;
			}
		}
	}
	return 0;
}

//�� ptPixelDatasָ���豸�� ibpp
static int DecodeMJPEG(PT_PixelDatas ptMjpegPixelDatas, PT_PixelDatas ptPixelDatas)
{
	unsigned char *buffer;	
	unsigned char *pucDest;
	int row_stride;
	int iBMPBpp = 0;
	int iWidth  = 0;
	int iHeight = 0;
	
	cinfo.err = jpeg_std_error(&jerr);	
	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, ptMjpegPixelDatas->aucPixelDatas, ptMjpegPixelDatas->iTotalBytes);
	
	//��ȡͷ��Ϣ	
	jpeg_read_header(&cinfo, TRUE);
	/*
	��� MJPEG ����ʹ�����淽����ȡ��С
	//���Զ�ȡ���ļ���С˵���� jpg
	if(0 < cinfo.image_width && 0 < cinfo.image_height)
	{
		DEBUG_PRINTF("get image size error \n");
		return -1;
	}
	if(3 == cinfo.num_components)
	{
		iBMPBpp = 24;
	}
	*/
	iBMPBpp = 24;
	// ���ý�ѹ����,����Ŵ���С
    cinfo.scale_num = cinfo.scale_denom = 1;
	
	//��ʼ����	
	jpeg_start_decompress(&cinfo);
	//һ�еĳ���	
	row_stride = cinfo.output_width * cinfo.output_components;
	buffer = malloc(row_stride);	
	if(! buffer)
	{
		DEBUG_PRINTF("malloc error \n");
		return -1;
	}
	iWidth = cinfo.output_width;
	iHeight = cinfo.output_height;
	
	pucDest = ptPixelDatas->aucPixelDatas;
	//ɨ�� 	
	while (cinfo.output_scanline < cinfo.output_height) 	
	{    
		/* jpeg_read_scanlines expects an array of pointers to scanlines.     
		 * Here the array is only one element long, but you could ask for     
		 * more than one scanline at a time if that's more convenient.     
		 */		
		 (void) jpeg_read_scanlines(&cinfo, &buffer, 1);		
		/* Assume put_scanline_someplace wants a pointer and sample count. */
		ConvertOneLine(iWidth, iBMPBpp, ptPixelDatas->iBpp, buffer, pucDest);
		//ת���Ժ�ĵ�2��
		pucDest += ptPixelDatas->iLineBytes;
	}
	free(buffer);	
	jpeg_finish_decompress(&cinfo);
	//�ͷ�	
	jpeg_destroy_decompress(&cinfo);
	return 0;
}

static int Mjpeg2rgbConvert(PT_VideoBuf ptVideoBufIn, PT_VideoBuf ptVideoBufOut)
{
	//��ȡ����תΪ  
	PT_PixelDatas ptPixelDatasIn  = &ptVideoBufIn->tPixelDatas;
    PT_PixelDatas ptPixelDatasOut = &ptVideoBufOut->tPixelDatas;

    ptPixelDatasOut->iWidth  = ptPixelDatasIn->iWidth;
    ptPixelDatasOut->iHeight = ptPixelDatasIn->iHeight;
    //ptPixelDatasOut->iBpp = 16;
    ptPixelDatasOut->iLineBytes  = ptPixelDatasOut->iWidth * ptPixelDatasOut->iBpp / 8;
    ptPixelDatasOut->iTotalBytes = ptPixelDatasOut->iLineBytes * ptPixelDatasOut->iHeight;

    if (! ptPixelDatasOut->aucPixelDatas)
    {
        ptPixelDatasOut->aucPixelDatas = malloc(ptPixelDatasOut->iTotalBytes);
    }
	//���� Mjpeg ͼ��
	if(DecodeMJPEG(ptPixelDatasIn, ptPixelDatasOut))
	{
		return -1;
	}
	return 0;

}

static int Mjpeg2rgbConvertExit(PT_VideoBuf ptVideoBufOut)
{
	if (ptVideoBufOut->tPixelDatas.aucPixelDatas)
    {
     	free(ptVideoBufOut->tPixelDatas.aucPixelDatas);
		ptVideoBufOut->tPixelDatas.aucPixelDatas = NULL;
	}
	return 0;
}

static T_VideoConvert tMjpeg2rgbVideoConvert = {
	.name        = "mjpeg",
	.isSupport   = Mjpeg2rgbIsSupport,
	.Convert     = Mjpeg2rgbConvert,
	.ConvertExit = Mjpeg2rgbConvertExit,
};

int InitMjpeg2rgb(void)
{
    return RegisterVideoConvert(&tMjpeg2rgbVideoConvert);
}

