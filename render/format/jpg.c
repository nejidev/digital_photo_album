#include <conf.h>
#include <pic_operation.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <setjmp.h>

static int isJPEGFormat(unsigned char *aFileHead, int iMemlen);
static int GetPixelDatasFrmJPEG(unsigned char *aFileHead, PT_PixelDatas ptPixelDatas);
static int FreePixelDatasForJPEG(PT_PixelDatas ptPixelDatas);

static T_PicFileParser g_tJPEGParser = {
	.name              = "jpg",
	.isSupport         = isJPEGFormat,
	.GetPixelDatas     = GetPixelDatasFrmJPEG,
	.FreePixelDatas    = FreePixelDatasForJPEG,
};

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

//支持返回1 否则返回0
static int isJPEGFormat(unsigned char *aFileHead, int iMemlen)
{
	cinfo.err = jpeg_std_error(&jerr);	
	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, aFileHead, iMemlen);
	
	//读取头信息	
	jpeg_read_header(&cinfo, TRUE);
	
	//可以读取到文件大小说明是 jpg
	if(0 < cinfo.image_width && 0 < cinfo.image_height)
	{
		return 1;
	}
	return 0;
}

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
		//这里忘了 颜色顺序转换 暂无 24BPP LCD 后期在测试
		memcpy(pucDest, pucSrc, iWidth*iBMPBpp/8);
	}
	else
	{
		for(i=0; i<iWidth; i++)
		{
			//JPG 中颜色排序是 RRGGBB
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

//由 ptPixelDatas指定设备的 ibpp
static int GetPixelDatasFrmJPEG(unsigned char *aFileHead, PT_PixelDatas ptPixelDatas)
{
	unsigned char *buffer;	
	unsigned char *pucDest;
	int row_stride;
	int iBMPBpp = 0;
	int iWidth  = 0;
	int iHeight = 0;
	if(3 == cinfo.num_components)
	{
		iBMPBpp = 24;
	}
	// 设置解压参数,比如放大、缩小
    cinfo.scale_num = cinfo.scale_denom = 1;
	
	//开始解码	
	jpeg_start_decompress(&cinfo);
	//一行的长度	
	row_stride = cinfo.output_width * cinfo.output_components;
	buffer = malloc(row_stride);	
	if(! buffer)
	{
		DEBUG_PRINTF("malloc error \n");
		return -1;
	}
	iWidth = cinfo.output_width;
	iHeight = cinfo.output_height;
	
	//ptPixelDatas设置参数
	ptPixelDatas->iWidth  = iWidth;
	ptPixelDatas->iHeight = iHeight;
	//ptPixelDatas->iBpp    = iBMPBpp;
	ptPixelDatas->aucPixelDatas = malloc(iWidth * iHeight * ptPixelDatas->iBpp / 8);
	ptPixelDatas->iLineBytes    = iWidth * ptPixelDatas->iBpp / 8;
	ptPixelDatas->iTotalBytes   = ptPixelDatas->iLineBytes * ptPixelDatas->iHeight;
	if(NULL == ptPixelDatas->aucPixelDatas)
	{
		DEBUG_PRINTF("malloc bmp pixelDatas \n");
		return -1;
	}
	pucDest = ptPixelDatas->aucPixelDatas;
	//扫描 	
	while (cinfo.output_scanline < cinfo.output_height) 	
	{    
		/* jpeg_read_scanlines expects an array of pointers to scanlines.     
		 * Here the array is only one element long, but you could ask for     
		 * more than one scanline at a time if that's more convenient.     
		 */		
		 (void) jpeg_read_scanlines(&cinfo, &buffer, 1);		
		/* Assume put_scanline_someplace wants a pointer and sample count. */
		ConvertOneLine(iWidth, iBMPBpp, ptPixelDatas->iBpp, buffer, pucDest);
		//转换以后的第2行
		pucDest += ptPixelDatas->iLineBytes;
	}
	free(buffer);	
	jpeg_finish_decompress(&cinfo);
	//释放	
	jpeg_destroy_decompress(&cinfo);
	return 0;
}

static int FreePixelDatasForJPEG(PT_PixelDatas ptPixelDatas)
{
	free(ptPixelDatas->aucPixelDatas);
	return 0;
}

int JPEGInitParse(void)
{
	return RegisterParse(&g_tJPEGParser);
}