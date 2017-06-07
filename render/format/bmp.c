#include <conf.h>
#include <pic_operation.h>
#include <stdlib.h>
#include <string.h>

//#pragma pack(push) /* ����ǰpack����ѹջ���� */
//#pragma pack(1)    /* �����ڽṹ�嶨��֮ǰʹ�� */

typedef struct tagBITMAPFILEHEADER { /* bmfh */
	unsigned short bfType; 
	unsigned long  bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned long  bfOffBits;
} __attribute__ ((packed)) BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER { /* bmih */
	unsigned long  biSize;
	unsigned long  biWidth;
	unsigned long  biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned long  biCompression;
	unsigned long  biSizeImage;
	unsigned long  biXPelsPerMeter;
	unsigned long  biYPelsPerMeter;
	unsigned long  biClrUsed;
	unsigned long  biClrImportant;
} __attribute__ ((packed)) BITMAPINFOHEADER;

//#pragma pack(pop) /* �ָ���ǰ��pack���� */

static int isBMPFormat(unsigned char *aFileHead, int iMemlen);
static int GetPixelDatasFrmBMP(unsigned char *aFileHead, PT_PixelDatas ptPixelDatas);
static int FreePixelDatasForBMP(PT_PixelDatas ptPixelDatas);

static T_PicFileParser g_tBMPParser = {
	.name           = "bmp",
	.isSupport      = isBMPFormat,
	.GetPixelDatas  = GetPixelDatasFrmBMP,
	.FreePixelDatas = FreePixelDatasForBMP,	
};

//֧�ַ���1 ���򷵻�0
static int isBMPFormat(unsigned char *aFileHead, int iMemlen)
{
	if(0x42 != aFileHead[0] && 0x4d != aFileHead[1])
	{
		return 0;
	}
	else
	{
		return 1;
	}
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
		//�������� ��ɫ˳��ת�� ���� 24BPP LCD �����ڲ���
		memcpy(pucDest, pucSrc, iWidth*iBMPBpp/8);
	}
	else
	{
		for(i=0; i<iWidth; i++)
		{
			//BMP ����ɫ������ BBGGRR
			dwBlue  = pucSrc[pos++];
			dwGreen = pucSrc[pos++];
			dwRed   = pucSrc[pos++];
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
//bmp �ļ�Ŀǰ��֧�� 24BPP ��֧��������ɫ
static int GetPixelDatasFrmBMP(unsigned char *aFileHead, PT_PixelDatas ptPixelDatas)
{
	BITMAPFILEHEADER *ptBITMAPFILEHEADER;
	BITMAPINFOHEADER *ptBITMAPINFOHEADER;
	int iWidth;
	int iHeight;
	int iBMPBpp;
	int y;
	unsigned char *pucSrc;
	unsigned char *pucDest;
	int iLineWidthAlign;
	int iLineWidthReal;

	ptBITMAPFILEHEADER = (BITMAPFILEHEADER *)aFileHead;
	ptBITMAPINFOHEADER = (BITMAPINFOHEADER *)(aFileHead + sizeof(BITMAPFILEHEADER));

	iWidth  = ptBITMAPINFOHEADER->biWidth;
	iHeight = ptBITMAPINFOHEADER->biHeight;
	iBMPBpp = ptBITMAPINFOHEADER->biBitCount;

	//��� BMP �ļ��Ƿ���24λ��ɫ
	if(24 != iBMPBpp)
	{
		DEBUG_PRINTF("iBMPBPP = %d \n", iBMPBpp);
		return -1;
	}

	//ptPixelDatas���ò���
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
	//�������ݵ� ptPixelDatas->aucPixelDatas
	//BMP�ļ����Ǵ����½ǿ�ʼ�� ��Ҫע�� ÿ���ǰ� 4�ֽڶ���
	iLineWidthReal  = iWidth * iBMPBpp / 8;
	iLineWidthAlign = (iLineWidthReal + 3) & (~3);
	pucSrc = aFileHead + ptBITMAPFILEHEADER->bfOffBits;
	//�Ƶ����һ��
	pucSrc = pucSrc + (iHeight-1)*iLineWidthAlign;
	pucDest = ptPixelDatas->aucPixelDatas;
	//���д���
	for(y=0; y<iHeight; y++)
	{
		ConvertOneLine(iWidth, iBMPBpp, ptPixelDatas->iBpp, pucSrc, pucDest);
		//BMP�ļ��д�������ת������2��
		pucSrc -= iLineWidthAlign;
		//ת���Ժ�ĵ�2��
		pucDest += ptPixelDatas->iLineBytes;
	}
	return 0;
}

static int FreePixelDatasForBMP(PT_PixelDatas ptPixelDatas)
{
	free(ptPixelDatas->aucPixelDatas);
	return 0;
}

int BmpInitParse(void)
{
	return RegisterParse(&g_tBMPParser);
}