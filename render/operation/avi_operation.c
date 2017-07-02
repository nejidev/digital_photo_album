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

//AVI 块读取大小
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

//实际在LCD上显示的
static T_PixelDatas    tIconPixelDatas;
//分配MJPEG
static T_VideoBuf tVideoBuf;
//分配转换以后使用
static T_VideoBuf tConvertBuf;
static PT_VideoConvert ptVideoConvert;

static int InitLcdMjpeg(void)
{
	//得到转换器
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

	//设置缩放大小
	tIconPixelDatas.iBpp        = iBpp;
	tIconPixelDatas.iHeight     = iYres * 8 / 10;
	tIconPixelDatas.iWidth      = iXres;
	tIconPixelDatas.iLineBytes  = tIconPixelDatas.iWidth * tIconPixelDatas.iBpp / 8;
	tIconPixelDatas.iTotalBytes = tIconPixelDatas.iLineBytes * tIconPixelDatas.iHeight;
	
	//分配缩放后 icon 图片的显存
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
	//检查长度不能超过文件大小
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

	//默认读取一次
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
	taviOpr.SecPerFrame = pAvihHeader->SecPerFrame; //得到帧间隔时间
	taviOpr.TotalFrame  = pAvihHeader->TotalFrame;	//得到总帧数  
	taviOpr.Streams     = pAvihHeader->Streams;	    //得到流数  
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
	//视频帧在前
	if(AVI_VIDS_STREAM == pStrhHeader->StreamType)			
	{
		//非MJPG视频流,不支持
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
		pVideoMem += pBmpHeader->BlockSize + 8; //偏移
		
		pListHeader = (LIST_HEADER*)(pVideoMem);
		if(AVI_LIST_ID != pListHeader->ListID)//是不含有音频帧的视频文件
		{
			taviOpr.SampleRate = 0;	//音频采样率
			taviOpr.Channels   = 0;	//音频通道数
			taviOpr.AudioType  = 0;	//音频格式
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
			taviOpr.SampleRate = pWavHeader->SampleRate;	//音频采样率
			taviOpr.Channels   = pWavHeader->Channels;	//音频通道数
			taviOpr.AudioType  = pWavHeader->FormatTag;	//音频格式
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
		taviOpr.SampleRate = pWavHeader->SampleRate;	//音频采样率
		taviOpr.Channels   = pWavHeader->Channels;	//音频通道数
		taviOpr.AudioType  = pWavHeader->FormatTag;	//音频格式
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
		//视频帧在前
		if(AVI_VIDS_STREAM != pStrhHeader->StreamType)					
		{
			DEBUG_PRINTF("STRH_HEADER error \n");
	 		return -1;
		}
		//非MJPG视频流,不支持
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

//查找 ID
static int SrarchID(unsigned char *buf, int size, unsigned char *id)
{
	int i;
	size -= 4;
	for(i = 0; i < size; i++)
	{
	   	if(buf[i]==id[0])
			if(buf[i+1]==id[1])
				if(buf[i+2]==id[2])	
					if(buf[i+3]==id[3]) return i;//找到"id"所在的位置	
	}
	return 0;		
}

static int GetStream(unsigned char *buf)
{
	taviOpr.StreamID   = MAKEWORD(buf + 2);	//得到流类型
	taviOpr.StreamSize = MAKEDWORD(buf + 4);	//得到流大小 
	if(taviOpr.StreamSize % 2)
	{
		taviOpr.StreamSize++;	//奇数加1(avix.StreamSize,必须是偶数)
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
		
	//查找 movi
	offset = SrarchID(pVideoMem, AVI_VIDEO_BUF_SIZE, (unsigned char*)"movi"); //寻找movi ID
	//指针跳过 movi 
	pVideoMem += offset + 4;
	
	//如果读取位置小于最大长度则还没有播放完毕
	while (taviOpr.total_size > taviOpr.read_pos)
	{
		if(! isPlay)
		{
			goto exit;
		}
		
		//解码
		if(GetStream(pVideoMem))
		{
			DEBUG_PRINTF("GetStream error\n");
			goto exit;
		}
		pVideoMem += 8;
		if(AVI_VIDS_FLAG == taviOpr.StreamID)	//视频流
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
			//1, MJPEG 解码
			tVideoBuf.iPixelFormat        = V4L2_PIX_FMT_MJPEG;
			tVideoBuf.tPixelDatas.iBpp    = 24;
			tVideoBuf.tPixelDatas.iHeight = taviOpr.Height;
			tVideoBuf.tPixelDatas.iWidth  = taviOpr.Width;
			tVideoBuf.tPixelDatas.iLineBytes  = tVideoBuf.tPixelDatas.iWidth * tVideoBuf.tPixelDatas.iBpp / 8;
			//tVideoBuf.tPixelDatas.iTotalBytes = tVideoBuf.tPixelDatas.iLineBytes * tVideoBuf.tPixelDatas.iHeight;
			tVideoBuf.tPixelDatas.aucPixelDatas = pVideoMem;
			tVideoBuf.tPixelDatas.iTotalBytes   = taviOpr.StreamSize;
			
			//2,MJPEG 转为 RGB
            iError = ptVideoConvert->Convert(&tVideoBuf, &tConvertBuf);
            if (iError)
            {
                DEBUG_PRINTF("Convert error!\n");
                goto exit;
            }
			
			//3,缩放
			PicZoom(&tConvertBuf.tPixelDatas, &tIconPixelDatas);

			//4,在LCD上显示
			VideoLCDShow(&tIconPixelDatas);
			
			//5,帧之间的间隔
			//usleep(taviOpr.SecPerFrame);
		}
		else 	//音频流
		{
			//暂不处理后期在加 MP3 解码
		}
		//6, 播放下一帧
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
	//测量LCD的信息
	GetDispResolution(&iXres, &iYres, &iBpp);
	
	//1,打开文件
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
	//2,启动播放进程
	//初始化成功创建子线程 将子项的GetInputEvent 传进来
	pthread_create(&tTreadID, NULL, EventTreadFunction, NULL);
	
	isPlay++;
	return 0;
}

int AVIPlayStop(void)
{
	isPlay = 0;
	//pthread_join 会发生死锁 直到线程安全退出
	pthread_join(tTreadID, NULL);
	if(tIconPixelDatas.aucPixelDatas)
	{
		free(tIconPixelDatas.aucPixelDatas);
	}
	return 0;
}

