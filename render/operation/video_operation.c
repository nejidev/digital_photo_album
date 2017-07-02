#include <conf.h>
#include <video_operation.h>
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

static pthread_t tTreadID;
static int iXres;
static int iYres;
static int iBpp;
static PT_VideoMem ptVideoMem;
static int isPlay = 0;
//ʵ����LCD����ʾ��
static T_PixelDatas    tIconPixelDatas;
static T_VideoDevice   tVideoDevice;
static PT_VideoConvert ptVideoConvert;
//��������ͷ
static T_VideoBuf tVideoBuf;
//����ת���Ժ�ʹ��
static T_VideoBuf tConvertBuf;
//��ǰʹ�õ�
static PT_VideoBuf ptVideoBufCur;
//���ձ���·��
static char JpgSavePath[256];

static void VideoLCDShow(PT_PixelDatas ptPixelDatas)
{
	PT_DispOpr disp = GetDefaultDispDev();
	memcpy(disp->pucDispMem + (iYres * 2 / 10 * disp->iLineWidth), ptPixelDatas->aucPixelDatas, ptPixelDatas->iTotalBytes);
}

static int JpegSave(PT_VideoBuf ptCameraBuf)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE * outfile;		        /* target file */
	JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
	int row_stride;		    /* physical row width in image buffer */
	int image_width;
	int image_height;
	int quality = 80;
	image_width  = ptCameraBuf->tPixelDatas.iWidth;
	image_height = ptCameraBuf->tPixelDatas.iHeight;
	
	/* Step 1: allocate and initialize JPEG compression object */
	cinfo.err = jpeg_std_error(&jerr);
	/* Now we can initialize the JPEG compression object. */
	jpeg_create_compress(&cinfo);

	/* Step 2: specify data destination (eg, a file) */
	if (NULL == (outfile = fopen(JpgSavePath, "wb"))) 
	{
		DEBUG_PRINTF("can't open %s\n", JpgSavePath);
		return -1;
	}
	jpeg_stdio_dest(&cinfo, outfile);
	
	/* Step 3: set parameters for compression */
	cinfo.image_width = image_width; 	/* image width and height, in pixels */
	cinfo.image_height = image_height;
	cinfo.input_components = 3;		/* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

	/* Step 4: Start compressor */
	jpeg_start_compress(&cinfo, TRUE);

	/* Step 5: while (scan lines remain to be written) */
	row_stride = image_width * 3;	/* JSAMPLEs per row in image_buffer */

	while (cinfo.next_scanline < cinfo.image_height) 
	{
    	row_pointer[0] = &ptCameraBuf->tPixelDatas.aucPixelDatas[cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	/* Step 6: Finish compression */
	jpeg_finish_compress(&cinfo);
	/* After finish_compress, we can close the output file. */
	fclose(outfile);

	/* Step 7: release JPEG compression object */
	jpeg_destroy_compress(&cinfo);
	return 0;
}

static void *EventTreadFunction(void *pVoid)
{
	int iError;
	while (1)
	{
		if(! isPlay)
		{
			goto exit;
		}
		ptVideoBufCur = &tVideoBuf;
		/* ��������ͷ���� */
        iError = tVideoDevice.ptOPr->GetFrame(&tVideoDevice, &tVideoBuf);
        if (iError)
        {
            DEBUG_PRINTF("GetFrame error! \n");
            goto exit;
        }

		//��������ʽ���� RGB565 ����Ҫת��
		if (tVideoDevice.ptOPr->GetFormat(&tVideoDevice) != V4L2_PIX_FMT_RGB565)
        {
            /* ת��ΪRGB */
            iError = ptVideoConvert->Convert(&tVideoBuf, &tConvertBuf);
            if (iError)
            {
                DEBUG_PRINTF("Convert error!\n");
                goto exit;
            }
            ptVideoBufCur = &tConvertBuf;
        }

		//ִ������
		PicZoom(&ptVideoBufCur->tPixelDatas, &tIconPixelDatas);
		
		//ֱ���� LCD ����ʾ
		VideoLCDShow(&tIconPixelDatas);
		
		iError = tVideoDevice.ptOPr->PutFrame(&tVideoDevice, &tVideoBuf);
        if (iError)
        {
            DEBUG_PRINTF("PutFrame for  error!\n");
            goto exit;
        }
	}
exit:
	
	return NULL;
}

static int VideoDevInit(void)
{
	int iError;
	static int initd = 0;
	if(initd)
	{
		return 0;
	}
	//��ʼ�� ����ͷ�豸
	iError = VideoDeviceInit(VIDEO_DEV, &tVideoDevice);
    if (iError)
    {
        DEBUG_PRINTF("VideoDeviceInit for %s error!\n", VIDEO_DEV);
        return -1;
    }

	//�õ�ת���� ����ͷ��ʽ תΪ LCD ��ʽ Ĭ���� 16λ 565 ��ʽ
	ptVideoConvert = GetVideoConvertForFormats(tVideoDevice.ptOPr->GetFormat(&tVideoDevice), V4L2_PIX_FMT_RGB565);
    if (NULL == ptVideoConvert)
    {
        DEBUG_PRINTF("can not support this format convert\n");
        return -1;
    }

	//����LCD����Ϣ
	GetDispResolution(&iXres, &iYres, &iBpp);
	memset(&tVideoBuf, 0, sizeof(tVideoBuf));
    memset(&tConvertBuf, 0, sizeof(tConvertBuf));
    tConvertBuf.iPixelFormat     = V4L2_PIX_FMT_RGB565;
    tConvertBuf.tPixelDatas.iBpp = iBpp;
	
	ptVideoMem = GetVideoMem(ID("sync"), 1);
	
	
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
	initd++;
	return 0;
}

int VideoPlayStart(void)
{
	int iError;
	VideoDevInit();
	/* ��������ͷ�豸 */
    iError = tVideoDevice.ptOPr->StartDevice(&tVideoDevice);
    if (iError)
    {
        DEBUG_PRINTF("StartDevice for %s error!\n", VIDEO_DEV);
        return -1;
    }
	isPlay++;
	//2,�������Ž���
	//��ʼ���ɹ��������߳� �������GetInputEvent ������
	pthread_create(&tTreadID, NULL, EventTreadFunction, NULL);
	return 0;
}

int VideoSaveJpg(char *dir)
{
	T_VideoBuf      tCameraBuf;
	PT_VideoConvert ptCameraConvert;
	int iError;
	time_t now;    
	struct tm *tm_now;    
	char    datetime[200];     
	time(&now);    
	tm_now = localtime(&now);    
	strftime(datetime, 200, "%Y%m%d%H%M%S", tm_now);
	snprintf(JpgSavePath, 256,  "%s/%s.jpg", dir, datetime);

	DEBUG_PRINTF("VideoSaveJpg save to %s \n", JpgSavePath);
	memset(&ptCameraConvert, 0, sizeof(ptCameraConvert));
	memset(&tCameraBuf, 0, sizeof(tCameraBuf));
	tCameraBuf.tPixelDatas.iBpp = 24;
	tCameraBuf.iPixelFormat     = V4L2_PIX_FMT_RGB32;
	//Ŀǰ��֧�� MJPEG ����ΪͼƬ�ļ�
	if(V4L2_PIX_FMT_MJPEG == tVideoDevice.ptOPr->GetFormat(&tVideoDevice))
	{
		isPlay = 0;
		//pthread_join �ᷢ������ ֱ���̰߳�ȫ�˳�
		pthread_join(tTreadID, NULL);
	
		/* ��������ͷ���� */
	    iError = tVideoDevice.ptOPr->GetFrame(&tVideoDevice, &tVideoBuf);
	    if (iError)
	    {
	        DEBUG_PRINTF("GetFrame error! \n");
	        return -1;
	    }
		//MJPEG תΪ RGB 24 BPP
		ptCameraConvert = GetVideoConvertForFormats(V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_RGB565);
	    if (NULL == ptCameraConvert)
	    {
	        DEBUG_PRINTF("can not support this format convert\n");
	        return -1;
	    }
		iError = ptCameraConvert->Convert(&tVideoBuf, &tCameraBuf);
        if (iError)
        {
            DEBUG_PRINTF("Convert error!\n");
            return -1;
        }
		if(JpegSave(&tCameraBuf))
		{
			DEBUG_PRINTF("JpegSave error! \n");
		}
		iError = tVideoDevice.ptOPr->PutFrame(&tVideoDevice, &tVideoBuf);
        if (iError)
        {
            DEBUG_PRINTF("PutFrame for  error!\n");
            return -1;
        }
 	}
	ptCameraConvert->ConvertExit(&tCameraBuf);
	VideoPlayStop();
	return 0;
}

int VideoPlayStop(void)
{
	isPlay = 0;
	//pthread_join �ᷢ������ ֱ���̰߳�ȫ�˳�
	pthread_join(tTreadID, NULL);
	
	//�ͷ� BMP ͼƬ�����ڴ�
	free(tIconPixelDatas.aucPixelDatas);
	
	//�ر�����ͷ
	tVideoDevice.ptOPr->StopDevice(&tVideoDevice);
	//tVideoDevice.ptOPr->ExitDevice(&tVideoDevice);
	return 0;
}

