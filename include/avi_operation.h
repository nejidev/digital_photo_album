#ifndef __AVI_OPERATION_H
#define __AVI_OPERATION_H
#include <conf.h>
#include <stdio.h>
#include <convert_manager.h>
#include <video_manager.h>

#define AVI_AUDIO_BUF_SIZE    1024*5		//定义avi解码时,音频buf大小.
#define AVI_VIDEO_BUF_SIZE    1024*60		//定义avi解码时,视频buf大小.

#define	 MAKEWORD(ptr)	(unsigned short)(((unsigned short)*((unsigned char*)(ptr))<<8)|(unsigned short)*(unsigned char*)((ptr)+1))
#define  MAKEDWORD(ptr)	(unsigned int)(((unsigned short)*(unsigned char*)(ptr)|(((unsigned short)*(unsigned char*)(ptr+1))<<8)|\
						(((unsigned short)*(unsigned char*)(ptr+2))<<16)|(((unsigned short)*(unsigned char*)(ptr+3))<<24))) 
						
#define AVI_RIFF_ID			0X46464952  /* R 0x52 I 0x49 F 0x46 F 0x46 */
#define AVI_AVI_ID			0X20495641
#define AVI_LIST_ID			0X5453494C  
#define AVI_HDRL_ID			0X6C726468	//信息块标志
#define AVI_MOVI_ID			0X69766F6D 	//数据块标志
#define AVI_STRL_ID			0X6C727473	//strl标志

#define AVI_AVIH_ID			0X68697661 	//avih子块∈AVI_HDRL_ID
#define AVI_STRH_ID			0X68727473	//strh(流头)子块∈AVI_STRL_ID
#define AVI_STRF_ID			0X66727473 	//strf(流格式)子块∈AVI_STRL_ID
#define AVI_STRD_ID			0X64727473 	//strd子块∈AVI_STRL_ID (可选的)

#define AVI_VIDS_STREAM		0X73646976	//视频流
#define AVI_AUDS_STREAM		0X73647561 	//音频流


#define AVI_VIDS_FLAG		0X6463		//视频流标志
#define AVI_AUDS_FLAG		0X7762 		//音频流标志
#define AVI_FORMAT_MJPG		0X47504A4D 

typedef struct{
	int iFd;        	// avi 文件打开句柄
	int read_pos;  		// 读取位置
	int total_size;     // avi 文件大小
	int SecPerFrame;	//视频帧间隔时间(单位为us)
	int TotalFrame;		//文件总帧数
	int Streams;		//包含的数据流种类个数,通常为2
	char VideoFLAG[4];  //视频帧标记,VideoFLAG="00dc"/"01dc"
	char AudioFLAG[4];  //音频帧标记,AudioFLAG="00wb"/"01wb"
	int Width;
	int Height;
	int SampleRate; //音频采样率
	int Channels;	 //音频通道数
	int AudioType;	 //音频格式
	unsigned short StreamID;			//流类型ID,StreamID=='dc'==0X6463 /StreamID=='wb'==0X7762
	unsigned int   StreamSize;			//流大小,必须是偶数,如果读取到为奇数,则加1.补为偶数.
} T_AVIOpr, *PT_AVIOpr;

//AVI 块信息
typedef struct
{	
	unsigned int RiffID;	/* RiffID=='RIFF' */
	unsigned int FileSize;	/* AVI文件大小(不包含最初的8字节,也RIFFID和FileSize不计算在内) */
	unsigned int AviID;		/* AviID=='AVI ' */
}AVI_HEADER;

//LIST 块信息
typedef struct
{	
	unsigned int ListID;	 /* ListID=='LIST' */
	unsigned int BlockSize;	 /* 块大小(不包含最初的8字节,也ListID和BlockSize不计算在内) */
	unsigned int ListType;	 /* LIST子块类型:hdrl(信息块)/movi(数据块)/idxl(索引块,非必须,是可选的) */
}LIST_HEADER;

//avih 子块信息
typedef struct
{	
	unsigned int BlockID;			//块标志:avih
	unsigned int BlockSize;			//块大小(不包含最初的8字节,也就是BlockID和BlockSize不计算在内)
	unsigned int SecPerFrame;		//视频帧间隔时间(单位为us)
	unsigned int MaxByteSec; 		//最大数据传输率,字节/秒
	unsigned int PaddingGranularity;//数据填充的粒度
	unsigned int Flags;				//AVI文件的全局标记，比如是否含有索引块等
	unsigned int TotalFrame;		//文件总帧数
	unsigned int InitFrames;  		//为交互格式指定初始帧数（非交互格式应该指定为0）
	unsigned int Streams;			//包含的数据流种类个数,通常为2
	unsigned int RefBufSize;			//建议读取本文件的缓存大小（应能容纳最大的块）默认可能是1M字节!!!
	unsigned int Width;				//图像宽
	unsigned int Height;			//图像高
	unsigned int Reserved[4];		//保留
}AVIH_HEADER;

//strh 流头子块信息(strh strl)
typedef struct
{	
	unsigned int BlockID;			//块标志:strh
	unsigned int BlockSize;			//块大小(不包含最初的8字节,也就是BlockID和BlockSize不计算在内)
	unsigned int StreamType;		//数据流种类，vids(0X73646976):视频;auds(0X73647561):音频
	unsigned int Handler;			//指定流的处理者，对于音视频来说就是解码器,比如MJPG/H264之类的.
	unsigned int Flags;  			//标记：是否允许这个流输出？调色板是否变化？
	unsigned short Priority;		//流的优先级（当有多个相同类型的流时优先级最高的为默认流）
	unsigned short Language;		//音频的语言代号
	unsigned int InitFrames;  		//为交互格式指定初始帧数
	unsigned int Scale;				//数据量, 视频每桢的大小或者音频的采样大小
	unsigned int Rate; 				//Scale/Rate=每秒采样数
	unsigned int Start;				//数据流开始播放的位置，单位为Scale
	unsigned int Length;			//数据流的数据量，单位为Scale
 	unsigned int RefBufSize;  		//建议使用的缓冲区大小
    unsigned int Quality;			//解压缩质量参数，值越大，质量越好
	unsigned int SampleSize;		//音频的样本大小
	struct					//视频帧所占的矩形 
	{				
	   	short Left;
		short Top;
		short Right;
		short Bottom;
	}Frame;				
}STRH_HEADER;

//BMP结构体
typedef struct
{
	unsigned int	 BmpSize;	 //bmp结构体大小,包含(BmpSize在内)
 	long Width;				     //图像宽
	long Height;			     //图像高
	unsigned short  Planes;		 //平面数，必须为1
	unsigned short  BitCount;	 //像素位数,0X0018表示24位
	unsigned int  Compression;	 //压缩类型，比如:MJPG/H264等
	unsigned int  SizeImage;	 //图像大小
	long XpixPerMeter;		     //水平分辨率
	long YpixPerMeter;		     //垂直分辨率
	unsigned int  ClrUsed;		 //实际使用了调色板中的颜色数,压缩格式中不使用
	unsigned int  ClrImportant;	 //重要的颜色
}BMP_HEADER;

//颜色表
typedef struct 
{
	unsigned char  rgbBlue;			//蓝色的亮度(值范围为0-255)
	unsigned char  rgbGreen; 		//绿色的亮度(值范围为0-255)
	unsigned char  rgbRed; 			//红色的亮度(值范围为0-255)
	unsigned char  rgbReserved;		//保留，必须为0
}AVIRGBQUAD;

//对于strh,如果是视频流,strf(流格式)使STRH_BMPHEADER块
typedef struct 
{
	unsigned int BlockID;	//块标志,strf==0X73747266
	unsigned int BlockSize;	//块大小(不包含最初的8字节,也就是BlockID和本BlockSize不计算在内)
	BMP_HEADER bmiHeader;  	//位图信息头
	AVIRGBQUAD bmColors[1];	//颜色表
}STRF_BMPHEADER;  

//对于strh,如果是音频流,strf(流格式)使STRH_WAVHEADER块
typedef struct 
{
	unsigned int BlockID;		 //块标志,strf==0X73747266
	unsigned int BlockSize;		 //块大小(不包含最初的8字节,也就是BlockID和本BlockSize不计算在内)
   	unsigned short FormatTag;	 //格式标志:0X0001=PCM,0X0055=MP3...
	unsigned short Channels;	 //声道数,一般为2,表示立体声
	unsigned int SampleRate; 	 //音频采样率
	unsigned int BaudRate;   	 //波特率 
	unsigned short BlockAlign; 	 //数据块对齐标志
	unsigned short Size;		 //该结构大小
}STRF_WAVHEADER;

int AVIPlayStart(char *avi);
int AVIPlayStop(void);
#endif

