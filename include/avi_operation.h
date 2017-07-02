#ifndef __AVI_OPERATION_H
#define __AVI_OPERATION_H
#include <conf.h>
#include <stdio.h>
#include <convert_manager.h>
#include <video_manager.h>

#define AVI_AUDIO_BUF_SIZE    1024*5		//����avi����ʱ,��Ƶbuf��С.
#define AVI_VIDEO_BUF_SIZE    1024*60		//����avi����ʱ,��Ƶbuf��С.

#define	 MAKEWORD(ptr)	(unsigned short)(((unsigned short)*((unsigned char*)(ptr))<<8)|(unsigned short)*(unsigned char*)((ptr)+1))
#define  MAKEDWORD(ptr)	(unsigned int)(((unsigned short)*(unsigned char*)(ptr)|(((unsigned short)*(unsigned char*)(ptr+1))<<8)|\
						(((unsigned short)*(unsigned char*)(ptr+2))<<16)|(((unsigned short)*(unsigned char*)(ptr+3))<<24))) 
						
#define AVI_RIFF_ID			0X46464952  /* R 0x52 I 0x49 F 0x46 F 0x46 */
#define AVI_AVI_ID			0X20495641
#define AVI_LIST_ID			0X5453494C  
#define AVI_HDRL_ID			0X6C726468	//��Ϣ���־
#define AVI_MOVI_ID			0X69766F6D 	//���ݿ��־
#define AVI_STRL_ID			0X6C727473	//strl��־

#define AVI_AVIH_ID			0X68697661 	//avih�ӿ��AVI_HDRL_ID
#define AVI_STRH_ID			0X68727473	//strh(��ͷ)�ӿ��AVI_STRL_ID
#define AVI_STRF_ID			0X66727473 	//strf(����ʽ)�ӿ��AVI_STRL_ID
#define AVI_STRD_ID			0X64727473 	//strd�ӿ��AVI_STRL_ID (��ѡ��)

#define AVI_VIDS_STREAM		0X73646976	//��Ƶ��
#define AVI_AUDS_STREAM		0X73647561 	//��Ƶ��


#define AVI_VIDS_FLAG		0X6463		//��Ƶ����־
#define AVI_AUDS_FLAG		0X7762 		//��Ƶ����־
#define AVI_FORMAT_MJPG		0X47504A4D 

typedef struct{
	int iFd;        	// avi �ļ��򿪾��
	int read_pos;  		// ��ȡλ��
	int total_size;     // avi �ļ���С
	int SecPerFrame;	//��Ƶ֡���ʱ��(��λΪus)
	int TotalFrame;		//�ļ���֡��
	int Streams;		//�������������������,ͨ��Ϊ2
	char VideoFLAG[4];  //��Ƶ֡���,VideoFLAG="00dc"/"01dc"
	char AudioFLAG[4];  //��Ƶ֡���,AudioFLAG="00wb"/"01wb"
	int Width;
	int Height;
	int SampleRate; //��Ƶ������
	int Channels;	 //��Ƶͨ����
	int AudioType;	 //��Ƶ��ʽ
	unsigned short StreamID;			//������ID,StreamID=='dc'==0X6463 /StreamID=='wb'==0X7762
	unsigned int   StreamSize;			//����С,������ż��,�����ȡ��Ϊ����,���1.��Ϊż��.
} T_AVIOpr, *PT_AVIOpr;

//AVI ����Ϣ
typedef struct
{	
	unsigned int RiffID;	/* RiffID=='RIFF' */
	unsigned int FileSize;	/* AVI�ļ���С(�����������8�ֽ�,ҲRIFFID��FileSize����������) */
	unsigned int AviID;		/* AviID=='AVI ' */
}AVI_HEADER;

//LIST ����Ϣ
typedef struct
{	
	unsigned int ListID;	 /* ListID=='LIST' */
	unsigned int BlockSize;	 /* ���С(�����������8�ֽ�,ҲListID��BlockSize����������) */
	unsigned int ListType;	 /* LIST�ӿ�����:hdrl(��Ϣ��)/movi(���ݿ�)/idxl(������,�Ǳ���,�ǿ�ѡ��) */
}LIST_HEADER;

//avih �ӿ���Ϣ
typedef struct
{	
	unsigned int BlockID;			//���־:avih
	unsigned int BlockSize;			//���С(�����������8�ֽ�,Ҳ����BlockID��BlockSize����������)
	unsigned int SecPerFrame;		//��Ƶ֡���ʱ��(��λΪus)
	unsigned int MaxByteSec; 		//������ݴ�����,�ֽ�/��
	unsigned int PaddingGranularity;//������������
	unsigned int Flags;				//AVI�ļ���ȫ�ֱ�ǣ������Ƿ����������
	unsigned int TotalFrame;		//�ļ���֡��
	unsigned int InitFrames;  		//Ϊ������ʽָ����ʼ֡�����ǽ�����ʽӦ��ָ��Ϊ0��
	unsigned int Streams;			//�������������������,ͨ��Ϊ2
	unsigned int RefBufSize;			//�����ȡ���ļ��Ļ����С��Ӧ���������Ŀ飩Ĭ�Ͽ�����1M�ֽ�!!!
	unsigned int Width;				//ͼ���
	unsigned int Height;			//ͼ���
	unsigned int Reserved[4];		//����
}AVIH_HEADER;

//strh ��ͷ�ӿ���Ϣ(strh strl)
typedef struct
{	
	unsigned int BlockID;			//���־:strh
	unsigned int BlockSize;			//���С(�����������8�ֽ�,Ҳ����BlockID��BlockSize����������)
	unsigned int StreamType;		//���������࣬vids(0X73646976):��Ƶ;auds(0X73647561):��Ƶ
	unsigned int Handler;			//ָ�����Ĵ����ߣ���������Ƶ��˵���ǽ�����,����MJPG/H264֮���.
	unsigned int Flags;  			//��ǣ��Ƿ�����������������ɫ���Ƿ�仯��
	unsigned short Priority;		//�������ȼ������ж����ͬ���͵���ʱ���ȼ���ߵ�ΪĬ������
	unsigned short Language;		//��Ƶ�����Դ���
	unsigned int InitFrames;  		//Ϊ������ʽָ����ʼ֡��
	unsigned int Scale;				//������, ��Ƶÿ��Ĵ�С������Ƶ�Ĳ�����С
	unsigned int Rate; 				//Scale/Rate=ÿ�������
	unsigned int Start;				//��������ʼ���ŵ�λ�ã���λΪScale
	unsigned int Length;			//������������������λΪScale
 	unsigned int RefBufSize;  		//����ʹ�õĻ�������С
    unsigned int Quality;			//��ѹ������������ֵԽ������Խ��
	unsigned int SampleSize;		//��Ƶ��������С
	struct					//��Ƶ֡��ռ�ľ��� 
	{				
	   	short Left;
		short Top;
		short Right;
		short Bottom;
	}Frame;				
}STRH_HEADER;

//BMP�ṹ��
typedef struct
{
	unsigned int	 BmpSize;	 //bmp�ṹ���С,����(BmpSize����)
 	long Width;				     //ͼ���
	long Height;			     //ͼ���
	unsigned short  Planes;		 //ƽ����������Ϊ1
	unsigned short  BitCount;	 //����λ��,0X0018��ʾ24λ
	unsigned int  Compression;	 //ѹ�����ͣ�����:MJPG/H264��
	unsigned int  SizeImage;	 //ͼ���С
	long XpixPerMeter;		     //ˮƽ�ֱ���
	long YpixPerMeter;		     //��ֱ�ֱ���
	unsigned int  ClrUsed;		 //ʵ��ʹ���˵�ɫ���е���ɫ��,ѹ����ʽ�в�ʹ��
	unsigned int  ClrImportant;	 //��Ҫ����ɫ
}BMP_HEADER;

//��ɫ��
typedef struct 
{
	unsigned char  rgbBlue;			//��ɫ������(ֵ��ΧΪ0-255)
	unsigned char  rgbGreen; 		//��ɫ������(ֵ��ΧΪ0-255)
	unsigned char  rgbRed; 			//��ɫ������(ֵ��ΧΪ0-255)
	unsigned char  rgbReserved;		//����������Ϊ0
}AVIRGBQUAD;

//����strh,�������Ƶ��,strf(����ʽ)ʹSTRH_BMPHEADER��
typedef struct 
{
	unsigned int BlockID;	//���־,strf==0X73747266
	unsigned int BlockSize;	//���С(�����������8�ֽ�,Ҳ����BlockID�ͱ�BlockSize����������)
	BMP_HEADER bmiHeader;  	//λͼ��Ϣͷ
	AVIRGBQUAD bmColors[1];	//��ɫ��
}STRF_BMPHEADER;  

//����strh,�������Ƶ��,strf(����ʽ)ʹSTRH_WAVHEADER��
typedef struct 
{
	unsigned int BlockID;		 //���־,strf==0X73747266
	unsigned int BlockSize;		 //���С(�����������8�ֽ�,Ҳ����BlockID�ͱ�BlockSize����������)
   	unsigned short FormatTag;	 //��ʽ��־:0X0001=PCM,0X0055=MP3...
	unsigned short Channels;	 //������,һ��Ϊ2,��ʾ������
	unsigned int SampleRate; 	 //��Ƶ������
	unsigned int BaudRate;   	 //������ 
	unsigned short BlockAlign; 	 //���ݿ�����־
	unsigned short Size;		 //�ýṹ��С
}STRF_WAVHEADER;

int AVIPlayStart(char *avi);
int AVIPlayStop(void);
#endif

