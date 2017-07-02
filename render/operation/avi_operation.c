#include <conf.h>
#include <video_operation.h>
#include <avi_operation.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <page_manager.h>
#include <disp_manager.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <jpeglib.h>
#include <setjmp.h>

//AVI ���ȡ��С
#define VIDEO_MEM_SIZE (1024*1024*30)

static pthread_t tTreadID;
static int iXres;
static int iYres;
static int iBpp;
static int isPlay = 0;
static unsigned char aVideoMem[VIDEO_MEM_SIZE];
static unsigned char *pVideoMem = aVideoMem;

static T_AVIOpr taviOpr;

static AVI_HEADER  *pAviHeader;
static LIST_HEADER *pListHeader;
static AVIH_HEADER *pAvihHeader;
static STRH_HEADER *pStrhHeader; 
static STRF_BMPHEADER *pBmpHeader;
static STRF_WAVHEADER *pWavHeader;

//ʵ����LCD����ʾ��
static T_PixelDatas    tIconPixelDatas;
//����MJPEG
static T_VideoBuf tVideoBuf;
//����ת���Ժ�ʹ��
static T_VideoBuf tConvertBuf;
static PT_VideoConvert ptVideoConvert;

static int InitLcdMjpeg(void)
{
	//�õ�ת����
	ptVideoConvert = GetVideoConvertForFormats(V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_RGB565);
    if (NULL == ptVideoConvert)
    {
        DEBUG_PRINTF("can not support this format convert\n");
        return -1;
    }
	memset(&tVideoBuf, 0, sizeof(tVideoBuf));
    memset(&tConvertBuf, 0, sizeof(tConvertBuf));
    tConvertBuf.iPixelFormat     = V4L2_PIX_FMT_RGB565;
    tConvertBuf.tPixelDatas.iBpp = iBpp;

	//�������Ŵ�С
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
	return 0;
}

static void PrintAVIInfo(void)
{
	int s = (int)(taviOpr.TotalFrame * taviOpr.SecPerFrame / 1000000);
	DEBUG_PRINTF("avi fream Width:%d Height:%d ", taviOpr.Width, taviOpr.Height);
	DEBUG_PRINTF("time len: %d:%d ", s / 60, s % 60);
	DEBUG_PRINTF("stream: %d ", taviOpr.Streams);
	if(1 == taviOpr.AudioType)
	{
		DEBUG_PRINTF("audio: format PCM ");
	}
	else if(0x55 == taviOpr.AudioType)
	{
		DEBUG_PRINTF("audio: format MP3 ");
	}
	else
	{
		DEBUG_PRINTF("audio: format undefine 0x%x", taviOpr.AudioType);
	}
	DEBUG_PRINTF("\n");
}

static int ReadAvi(int readLen)
{
	//��鳤�Ȳ��ܳ����ļ���С
	readLen = min(taviOpr.total_size - taviOpr.read_pos, readLen);
	if(readLen != read(taviOpr.iFd, aVideoMem, readLen))
	{
		DEBUG_PRINTF("ReadAvi error read len :%d \n", readLen);
		return -1;
	}
	return 0;
}

static int OpenAVI(char *avi)
{
	struct stat aviStat;
	taviOpr.iFd = open(avi, O_RDONLY);
	if(0 > taviOpr.iFd)
	{
		DEBUG_PRINTF("OpenAVI %s error \n", avi);
	}
	if(fstat(taviOpr.iFd, &aviStat))
	{
		DEBUG_PRINTF("fstat %s error \n", avi);
	}
	taviOpr.read_pos   = 0;
	taviOpr.total_size = aviStat.st_size;

	//Ĭ�϶�ȡһ��
	if(ReadAvi(VIDEO_MEM_SIZE))
	{
		DEBUG_PRINTF("ReadAvi %s error \n", avi);
		return -1;
	}
	
	//RIFF
	pAviHeader = (AVI_HEADER *)pVideoMem;
	if(AVI_RIFF_ID != pAviHeader->RiffID || AVI_AVI_ID != pAviHeader->AviID)
	{
		DEBUG_PRINTF("file not avi fomart %s \n", avi);
		return -1;
	}
	pVideoMem += sizeof(AVI_HEADER);
	
	//LIST_HEADER
	pListHeader = (LIST_HEADER *)pVideoMem;
	if(AVI_LIST_ID != pListHeader->ListID || AVI_HDRL_ID != pListHeader->ListType)
	{
		DEBUG_PRINTF("LIST_HEADER error \n");
		return -1;
	}
	pVideoMem += sizeof(LIST_HEADER);
	
	//AVIH_HEADER
	pAvihHeader = (AVIH_HEADER *)pVideoMem;
	if(AVI_AVIH_ID != pAvihHeader->BlockID)
	{
		DEBUG_PRINTF("AVIH_HEADER error \n");
		return -1; 
	}
	taviOpr.SecPerFrame = pAvihHeader->SecPerFrame; //�õ�֡���ʱ��
	taviOpr.TotalFrame  = pAvihHeader->TotalFrame;	//�õ���֡��  
	taviOpr.Streams     = pAvihHeader->Streams;	    //�õ�����  
	pVideoMem += pAvihHeader->BlockSize + 8;
	//pVideoMem += sizeof(AVIH_HEADER);

	//LIST_HEADER
	pListHeader =(LIST_HEADER *)(pVideoMem);
 	if(AVI_LIST_ID != pListHeader->ListID || AVI_STRL_ID != pListHeader->ListType)
 	{
 		DEBUG_PRINTF("LIST_HEADER error \n");
 		return -1;
 	}
	pStrhHeader = (STRH_HEADER *)(pVideoMem + 12);
	if(AVI_STRH_ID != pStrhHeader->BlockID)
	{
		DEBUG_PRINTF("STRH_HEADER error \n");
 		return -1;
	}
	//��Ƶ֡��ǰ
	if(AVI_VIDS_STREAM == pStrhHeader->StreamType)			
	{
		//��MJPG��Ƶ��,��֧��
		if(AVI_FORMAT_MJPG != pStrhHeader->Handler)
		{
			DEBUG_PRINTF("StreamType no is MJPEG error \n");
 			return -1;
		}
		strncpy(taviOpr.VideoFLAG, "00dc", 4);
		strncpy(taviOpr.AudioFLAG, "01wb", 4);
		
		pBmpHeader = (STRF_BMPHEADER*)(pVideoMem + 12 + pStrhHeader->BlockSize + 8);//strf
		
		if(AVI_STRF_ID != pBmpHeader->BlockID)
		{
			DEBUG_PRINTF("STRF_BMPHEADER error \n");
 			return -1;
		}
		if(AVI_FORMAT_MJPG != pBmpHeader->bmiHeader.Compression)
		{
			DEBUG_PRINTF("StreamType no is MJPEG error \n");
 			return -1;
		}
		taviOpr.Width  = pBmpHeader->bmiHeader.Width;
		taviOpr.Height = pBmpHeader->bmiHeader.Height; 
		pVideoMem += pBmpHeader->BlockSize + 8; //ƫ��
		
		pListHeader = (LIST_HEADER*)(pVideoMem);
		if(AVI_LIST_ID != pListHeader->ListID)//�ǲ�������Ƶ֡����Ƶ�ļ�
		{
			taviOpr.SampleRate = 0;	//��Ƶ������
			taviOpr.Channels   = 0;	//��Ƶͨ����
			taviOpr.AudioType  = 0;	//��Ƶ��ʽ
		}
		else
		{
			if(AVI_STRL_ID != pListHeader->ListType)
			{
				DEBUG_PRINTF("AVI_STRL_ID error \n");
 				return -1;
			}
			pStrhHeader = (STRH_HEADER*)(pVideoMem + 12);
			if(AVI_STRH_ID != pStrhHeader->BlockID || AVI_AUDS_STREAM != pStrhHeader->StreamType)
			{
				DEBUG_PRINTF("STRH_HEADER error \n");
 				return -1;
			}
			pWavHeader = (STRF_WAVHEADER*)(pVideoMem + 12 + pStrhHeader->BlockSize + 8);//strf
			if(AVI_STRF_ID != pWavHeader->BlockID)
			{
				DEBUG_PRINTF("STRF_WAVHEADER error \n");
 				return -1;
			}
			taviOpr.SampleRate = pWavHeader->SampleRate;	//��Ƶ������
			taviOpr.Channels   = pWavHeader->Channels;	//��Ƶͨ����
			taviOpr.AudioType  = pWavHeader->FormatTag;	//��Ƶ��ʽ
		}
	}
	else
	{
		strncpy(taviOpr.VideoFLAG, "01dc", 4);
		strncpy(taviOpr.AudioFLAG, "00wb", 4);
		pWavHeader = (STRF_WAVHEADER*)(pVideoMem + 12 + pStrhHeader->BlockSize + 8);//strf
		if(AVI_STRF_ID != pWavHeader->BlockID)
		{
			DEBUG_PRINTF("STRF_WAVHEADER error \n");
 			return -1;
		}
		taviOpr.SampleRate = pWavHeader->SampleRate;	//��Ƶ������
		taviOpr.Channels   = pWavHeader->Channels;	//��Ƶͨ����
		taviOpr.AudioType  = pWavHeader->FormatTag;	//��Ƶ��ʽ
		pVideoMem += pAvihHeader->BlockSize + 8;
		
		//LIST_HEADER
		pListHeader =(LIST_HEADER *)(pVideoMem);
	 	if(AVI_LIST_ID != pListHeader->ListID || AVI_STRL_ID != pListHeader->ListType)
	 	{
	 		DEBUG_PRINTF("LIST_HEADER error \n");
	 		return -1;
	 	}
		pStrhHeader = (STRH_HEADER *)(pVideoMem + 12);
		if(AVI_STRH_ID != pStrhHeader->BlockID)
		{
			DEBUG_PRINTF("STRH_HEADER error \n");
	 		return -1;
		}
		//��Ƶ֡��ǰ
		if(AVI_VIDS_STREAM != pStrhHeader->StreamType)					
		{
			DEBUG_PRINTF("STRH_HEADER error \n");
	 		return -1;
		}
		//��MJPG��Ƶ��,��֧��
		if(AVI_FORMAT_MJPG != pStrhHeader->Handler)
		{
			DEBUG_PRINTF("StreamType no is MJPEG error \n");
 			return -1;
		}
		pBmpHeader = (STRF_BMPHEADER*)(pVideoMem + 12 + pStrhHeader->BlockSize + 8);//strf
		if(AVI_STRF_ID != pBmpHeader->BlockID)
		{
			DEBUG_PRINTF("STRF_BMPHEADER error \n");
 			return -1;
		}
		if(AVI_FORMAT_MJPG != pBmpHeader->bmiHeader.Compression)
		{
			DEBUG_PRINTF("StreamType no is MJPEG error \n");
 			return -1;
		}
		taviOpr.Width  = pBmpHeader->bmiHeader.Width;
		taviOpr.Height = pBmpHeader->bmiHeader.Height; 
		
	}
	PrintAVIInfo();
	return 0;
}

static int CloseAVI(void)
{
	close(taviOpr.iFd);
	return 0;
}

static void VideoLCDShow(PT_PixelDatas ptPixelDatas)
{
	PT_DispOpr disp = GetDefaultDispDev();
	memcpy(disp->pucDispMem + (iYres * 2 / 10 * disp->iLineWidth), ptPixelDatas->aucPixelDatas, ptPixelDatas->iTotalBytes);
}

//���� ID
static int SrarchID(unsigned char *buf, int size, unsigned char *id)
{
	int i;
	size -= 4;
	for(i = 0; i < size; i++)
	{
	   	if(buf[i]==id[0])
			if(buf[i+1]==id[1])
				if(buf[i+2]==id[2])	
					if(buf[i+3]==id[3]) return i;//�ҵ�"id"���ڵ�λ��	
	}
	return 0;		
}

static int GetStream(unsigned char *buf)
{
	taviOpr.StreamID   = MAKEWORD(buf + 2);	//�õ�������
	taviOpr.StreamSize = MAKEDWORD(buf + 4);	//�õ�����С 
	if(taviOpr.StreamSize % 2)
	{
		taviOpr.StreamSize++;	//������1(avix.StreamSize,������ż��)
	}
	
	if(AVI_VIDS_FLAG != taviOpr.StreamID && AVI_AUDS_FLAG != taviOpr.StreamID)
	{
		return -1;
	}
	return 0;
}

static void *EventTreadFunction(void *pVoid)
{
	int iError;
	unsigned char *framebuf;
	int offset;
		
	//���� movi
	offset = SrarchID(pVideoMem, AVI_VIDEO_BUF_SIZE, (unsigned char*)"movi"); //Ѱ��movi ID
	//ָ������ movi 
	pVideoMem += offset + 4;
	
	//�����ȡλ��С����󳤶���û�в������
	while (taviOpr.total_size > taviOpr.read_pos)
	{
		if(! isPlay)
		{
			goto exit;
		}
		
		//����
		if(GetStream(pVideoMem))
		{
			DEBUG_PRINTF("GetStream error\n");
			goto exit;
		}
		pVideoMem += 8;
		if(AVI_VIDS_FLAG == taviOpr.StreamID)	//��Ƶ��
		{
			if(! framebuf)
			{
				framebuf = malloc(taviOpr.StreamSize);
				if(! framebuf)
				{
					DEBUG_PRINTF("malloc error \n");
					goto exit;
				}
			}
			//1, MJPEG ����
			tVideoBuf.iPixelFormat        = V4L2_PIX_FMT_MJPEG;
			tVideoBuf.tPixelDatas.iBpp    = 24;
			tVideoBuf.tPixelDatas.iHeight = taviOpr.Height;
			tVideoBuf.tPixelDatas.iWidth  = taviOpr.Width;
			tVideoBuf.tPixelDatas.iLineBytes  = tVideoBuf.tPixelDatas.iWidth * tVideoBuf.tPixelDatas.iBpp / 8;
			//tVideoBuf.tPixelDatas.iTotalBytes = tVideoBuf.tPixelDatas.iLineBytes * tVideoBuf.tPixelDatas.iHeight;
			tVideoBuf.tPixelDatas.aucPixelDatas = pVideoMem;
			tVideoBuf.tPixelDatas.iTotalBytes   = taviOpr.StreamSize;
			
			//2,MJPEG תΪ RGB
            iError = ptVideoConvert->Convert(&tVideoBuf, &tConvertBuf);
            if (iError)
            {
                DEBUG_PRINTF("Convert error!\n");
                goto exit;
            }
			
			//3,����
			PicZoom(&tConvertBuf.tPixelDatas, &tIconPixelDatas);

			//4,��LCD����ʾ
			VideoLCDShow(&tIconPixelDatas);
			
			//5,֮֡��ļ��
			//usleep(taviOpr.SecPerFrame);
		}
		else 	//��Ƶ��
		{
			//�ݲ���������ڼ� MP3 ����
		}
		//6, ������һ֡
		pVideoMem += taviOpr.StreamSize;
	}
exit:
	if(framebuf)
	{
		free(framebuf);
	}
	return NULL;
}

int AVIPlayStart(char *avi)
{
	if(0 < isPlay)
	{
		DEBUG_PRINTF("AVIPlayStart error \n");
		return -1;
	}
	//����LCD����Ϣ
	GetDispResolution(&iXres, &iYres, &iBpp);
	
	//1,���ļ�
	if(OpenAVI(avi))
	{
		DEBUG_PRINTF("AVIPlayStart error \n");
		return -1;
	}
	if(InitLcdMjpeg())
	{
		DEBUG_PRINTF("AVIPlayStart InitLcdMjpeg error \n");
		return -1;
	}
	//2,�������Ž���
	//��ʼ���ɹ��������߳� �������GetInputEvent ������
	pthread_create(&tTreadID, NULL, EventTreadFunction, NULL);
	
	isPlay++;
	return 0;
}

int AVIPlayStop(void)
{
	isPlay = 0;
	//pthread_join �ᷢ������ ֱ���̰߳�ȫ�˳�
	pthread_join(tTreadID, NULL);
	if(tIconPixelDatas.aucPixelDatas)
	{
		free(tIconPixelDatas.aucPixelDatas);
	}
	return 0;
}

