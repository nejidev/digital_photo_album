#include <conf.h>
#include <pic_operation.h>
#include <gif_operation.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <render.h>
#include <page_manager.h>
#include <disp_manager.h>
#include <pthread.h>

static GifByteType  *Extension;
static GifRowType   *ScreenBuffer;
static GifFileType  *GifFile;
static GifRecordType RecordType;
static int GifWidth, GifHeight;
static T_PixelDatas  tOriginIconPixelDatas;
static T_PixelDatas  tIconPixelDatas;
static int           gifPlayDelayMs = 0;
static pthread_t     tTreadID;
static int           gifBPP = 24;
static int iXres;
static int iYres;
static int iBpp;
static PT_VideoMem ptVideoMem;
static int isPlay = 0;

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
			//GIF ����ɫ������ RRGGBB
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

static void DrawGif(ColorMapObject *ColorMap, GifRowType *ScreenBuffer, int ScreenWidth, int ScreenHeight)
{
	int i, j;
    GifRowType GifRow;
    GifColorType *ColorMapEntry;
	unsigned char *buffer;
	unsigned char *pucDest;
	unsigned char *p;
	buffer = malloc(ScreenWidth * ScreenHeight * gifBPP);
	pucDest = tOriginIconPixelDatas.aucPixelDatas;
	
	if(! buffer)
	{
		DEBUG_PRINTF("malloc \n");
	}
    for (i = 0; i < ScreenHeight; i++) 
	{
        GifRow = ScreenBuffer[i];
		p = buffer;
        for (j = 0; j < ScreenWidth; j++) 
		{
            ColorMapEntry = &ColorMap->Colors[GifRow[j]];
            *p = ColorMapEntry->Red;
			p++;
            *p = ColorMapEntry->Green;
			p++;
            *p = ColorMapEntry->Blue;
			p++;
        }
		//ת��һ��
		ConvertOneLine(ScreenWidth, gifBPP, tOriginIconPixelDatas.iBpp, buffer, pucDest);
		pucDest += tOriginIconPixelDatas.iLineBytes;
    }
	free(buffer);
	
	//ִ������
	PicZoom(&tOriginIconPixelDatas, &tIconPixelDatas);
				
	//д���Դ�
	PicMerge(0, iYres * 2 / 10, &tIconPixelDatas, &ptVideoMem->tPixelDatas);	
	FlushVideoMemToDevSync(ptVideoMem);
	ShowHistoryMouse();
}

static void *EventTreadFunction(void *pVoid)
{
	int Row, Col, Width, Height, i, j, Count, ExtCode; 
	ColorMapObject *ColorMap;
	int	InterlacedOffset[] = { 0, 4, 2, 1 }, /* The way Interlaced image should. */	
		InterlacedJumps[]  = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */
	//�õ�˽�����ݵõ�FILE ���
	fpos_t  pos;
	GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;
	FILE *pFD = Private->File;
	fgetpos(pFD, &pos);
	
	while (1)
	{
		/* Scan the content of the GIF file and load the image(s) in: */
		do
		{
			if(! isPlay)
			{
				//printf("play is stop \n");
				//pthread_exit(NULL);
				//return ;
			}
			//ʵ�ֵ��� fread ��ȡ�ļ�
			if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR) 
			{
				printf("giferr %d \n", GifFile->Error);
			}
			switch (RecordType) 
			{
				case IMAGE_DESC_RECORD_TYPE:
				if (DGifGetImageDesc(GifFile) == GIF_ERROR) 
				{
					printf("giferr %d \n", GifFile->Error);
				}
				Row = GifFile->Image.Top; /* Image Position relative to Screen. */
				Col = GifFile->Image.Left;
				Width = GifFile->Image.Width;
				Height = GifFile->Image.Height;
				if (GifFile->Image.Left + GifFile->Image.Width > GifFile->SWidth ||
				   GifFile->Image.Top + GifFile->Image.Height > GifFile->SHeight) 
				{
					printf("Image is not confined to screen dimension, aborted.\n");
				}
				if (GifFile->Image.Interlace) 
				{
					/* Need to perform 4 passes on the images: */
					for (Count = i = 0; i < 4; i++)
					{
						for (j = Row + InterlacedOffset[i]; j < Row + Height; j += InterlacedJumps[i]) 
						{
							if (DGifGetLine(GifFile, &ScreenBuffer[j][Col],Width) == GIF_ERROR) 
							{
								printf("giferr %d \n", GifFile->Error);
							}
						}
					}
				}
				else 
				{
					for (i = 0; i < Height; i++) 
					{
						if (DGifGetLine(GifFile, &ScreenBuffer[Row++][Col], Width) == GIF_ERROR) 
						{
							printf("giferr %d \n", GifFile->Error);
						}
					}
				}
				/* Lets dump it - set the global variables required and do it: */
				ColorMap = (GifFile->Image.ColorMap ? GifFile->Image.ColorMap : GifFile->SColorMap);
				if (ColorMap == NULL) 
				{
					fprintf(stderr, "Gif Image does not have a colormap\n");
				}

				DrawGif(ColorMap, ScreenBuffer, GifFile->SWidth, GifFile->SHeight);
			break;
			case EXTENSION_RECORD_TYPE:
				/* Skip any extension blocks in file: */
				if (DGifGetExtension(GifFile, &ExtCode, &Extension) == GIF_ERROR) 
				{
					printf("giferr %d \n", GifFile->Error);
				}
				while (Extension != NULL) 
				{
					if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR) 
					{
						printf("giferr %d \n", GifFile->Error);
					}
				}
				//printf("gif EXTENSION_RECORD_TYPE \n");
			break;
			case TERMINATE_RECORD_TYPE:
			break;
			default:		    /* Should be trapped by DGifGetRecordType. */
			break;
		}
		} while (RecordType != TERMINATE_RECORD_TYPE);
		//GIF���ż�� us
		//usleep(gifPlayDelayMs);
		printf("read play gif \n");
		//��������ָ��λ��ѭ������
		fsetpos(pFD, &pos);
	}
}

int GIFisSupport(char *aFileName)
{
	int err;
	GifFile = DGifOpenFileName(aFileName, &err);	
	if(0 == GifFile->SHeight || 0 == GifFile->SWidth) 
	{		
		DEBUG_PRINTF("file not is gif \n");
		return -1;
	}
	GifWidth = GifFile->SWidth;
	GifHeight = GifFile->SHeight;
	return 0;
}

int GIFPlayStart(void)
{	
	int Size, i;
		
	ptVideoMem = GetVideoMem(ID("sync"), 1);
	
	//����LCD����Ϣ
	GetDispResolution(&iXres, &iYres, &iBpp);
	
	//�������Ŵ�С
	tOriginIconPixelDatas.iBpp  = iBpp;
	tIconPixelDatas.iBpp        = iBpp;
	tIconPixelDatas.iHeight     = iYres * 8 / 10;
	tIconPixelDatas.iWidth      = iXres;
	tIconPixelDatas.iLineBytes  = tIconPixelDatas.iWidth * tIconPixelDatas.iBpp / 8;
	tIconPixelDatas.iTotalBytes = tIconPixelDatas.iLineBytes * tIconPixelDatas.iHeight;
	

	//�������ź� icon ͼƬ���Դ�
	tIconPixelDatas.aucPixelDatas = (unsigned char *)malloc(tIconPixelDatas.iTotalBytes);
	if(NULL == tIconPixelDatas.aucPixelDatas)
	{
		DEBUG_PRINTF("malloc tIconPixelDatas error \n");
		return -1;
	}
	//ptPixelDatas���ò���
	tOriginIconPixelDatas.iWidth  = GifWidth;
	tOriginIconPixelDatas.iHeight = GifHeight;
	tOriginIconPixelDatas.aucPixelDatas = malloc(GifWidth * GifHeight * tOriginIconPixelDatas.iBpp / 8);
	tOriginIconPixelDatas.iLineBytes    = GifWidth * tOriginIconPixelDatas.iBpp / 8;
	tOriginIconPixelDatas.iTotalBytes   = tOriginIconPixelDatas.iLineBytes * tOriginIconPixelDatas.iHeight;
	if(! tOriginIconPixelDatas.aucPixelDatas)
	{
		DEBUG_PRINTF("malloc bmp pixelDatas \n");
		return -1;
	}
	/* 
     * Allocate the screen as vector of column of rows. Note this
     * screen is device independent - it's the screen defined by the
     * GIF file parameters.
	 * ����GIF���������ñ�����ɫ ��߱�����ɫ����GIF 
     */
    if ((ScreenBuffer = (GifRowType *)malloc(GifFile->SHeight * sizeof(GifRowType))) == NULL)
    {
	    DEBUG_PRINTF("Failed to allocate memory required, aborted.");
		return -1;
    }
    Size = GifFile->SWidth * sizeof(GifPixelType);/* Size in bytes one row.*/
    if ((ScreenBuffer[0] = (GifRowType) malloc(Size)) == NULL) /* First row. */
    {
		DEBUG_PRINTF("Failed to allocate memory required, aborted.");
		return -1;
    }
    for (i = 0; i < GifFile->SWidth; i++)  /* Set its color to BackGround. */
		ScreenBuffer[0][i] = GifFile->SBackGroundColor;
    for (i = 1; i < GifFile->SHeight; i++) 
	{
		/* Allocate the other rows, and set their color to background too: */
		if ((ScreenBuffer[i] = (GifRowType) malloc(Size)) == NULL)
			printf("Failed to allocate memory required, aborted.");
		memcpy(ScreenBuffer[i], ScreenBuffer[0], Size);
    }
	
	isPlay++;
	//2,�������Ž���
	//��ʼ���ɹ��������߳� �������GetInputEvent ������
	pthread_create(&tTreadID, NULL, EventTreadFunction, NULL);

	return 0;
}

int GIFPlayStop(void)
{
	int err;
	isPlay = 0;
	if(pthread_cancel(tTreadID))
	{
		DEBUG_PRINTF("pthread_cancel err \n");    
		return -1;
	}
	
	printf("gif play stop \n");
	//�ͷ� BMP ͼƬ�����ڴ�
	free(tOriginIconPixelDatas.aucPixelDatas);
	free(tIconPixelDatas.aucPixelDatas);
	free(ScreenBuffer);   
	if (DGifCloseFile(GifFile, &err) == GIF_ERROR) 	
	{		
		DEBUG_PRINTF("giferr %d \n", GifFile->Error);    
		return -1;
	}
	return 0;
}
