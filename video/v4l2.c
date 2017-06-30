#include <video_manager.h>
#include <disp_manager.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

//#define USE_FMT_YUYV

static T_VideoOpr tV4l2VideoOpr;
static int g_aiSupportedFormats[] = {V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_RGB565};

static int V4l2GetFrameForReadWrite(PT_VideoDevice ptVideoDevice, PT_VideoBuf ptVideoBuf);
static int V4l2PutFrameForReadWrite(PT_VideoDevice ptVideoDevice, PT_VideoBuf ptVideoBuf);

static int PixelformatIsSupport(int pixelFormat)
{
	int i;
	for(i = 0; i < sizeof(g_aiSupportedFormats)/sizeof(g_aiSupportedFormats[0]); i++)
	{
		if(pixelFormat == g_aiSupportedFormats[i])
		{
			return 1;
		}
	}
	return 0;
}

/* 参考 luvcview */

/* open
 * VIDIOC_QUERYCAP 确定它是否视频捕捉设备,支持哪种接口(streaming/read,write)
 * VIDIOC_ENUM_FMT 查询支持哪种格式
 * VIDIOC_S_FMT    设置摄像头使用哪种格式
 * VIDIOC_REQBUFS  申请buffer
 对于 streaming:
 * VIDIOC_QUERYBUF 确定每一个buffer的信息 并且 mmap
 * VIDIOC_QBUF     放入队列
 * VIDIOC_STREAMON 启动设备
 * poll            等待有数据
 * VIDIOC_DQBUF    从队列中取出
 * 处理....
 * VIDIOC_QBUF     放入队列
 * ....
 对于read,write:
    read
    处理....
    read
 * VIDIOC_STREAMOFF 停止设备
 *
 */
static int V4l2InitDevice(char *strDevName, PT_VideoDevice ptVideoDevice)
{
	int i;
    int ret = 0;
	struct v4l2_capability tCap;
	struct v4l2_format tFmt;
	struct v4l2_fmtdesc tDescFmt;
	struct v4l2_requestbuffers tRb;
	struct v4l2_buffer tBuf;
	int iLcdWidth;
    int iLcdHeigt;
    int iLcdBpp;

	GetDispResolution(&iLcdWidth, &iLcdHeigt, &iLcdBpp);
	
    if((ptVideoDevice->iFd = open(strDevName, O_RDWR)) == -1) 
	{
		DEBUG_PRINTF("V4l2InitDevice open %s error \n", strDevName);
		goto fatal;    
	}
    memset(&tCap, 0, sizeof(struct v4l2_capability));
    ret = ioctl(ptVideoDevice->iFd, VIDIOC_QUERYCAP, &tCap);
    if (ret < 0) 
	{
		DEBUG_PRINTF("unable to query device.\n");
		goto fatal;
    }

    if ((tCap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) 
	{
		DEBUG_PRINTF("video capture not supported.\n");
		goto fatal;;
    }
	//检测 支持的 控制方式
	if(tCap.capabilities & V4L2_CAP_STREAMING)
	{
	    DEBUG_PRINTF("support streaming i/o\n");
	}
	if(tCap.capabilities & V4L2_CAP_READWRITE) 
	{
	    DEBUG_PRINTF("support read i/o\n");
	}
	/* get format */
	//获取支持的格式 YUYV 、 MJPEG
	memset(&tDescFmt, 0, sizeof(tDescFmt));
	tDescFmt.index = 0;
	tDescFmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while ((ret = ioctl(ptVideoDevice->iFd, VIDIOC_ENUM_FMT, &tDescFmt)) == 0) 
	{
		//DEBUG_PRINTF("tDescFmt.pixelformat: 0x%x \n", tDescFmt.pixelformat);
		if(PixelformatIsSupport(tDescFmt.pixelformat))
		{
			//强制使用 yuv 模式
			#ifdef USE_FMT_YUYV
			if(V4L2_PIX_FMT_YUYV == tDescFmt.pixelformat)
			{
			#endif
				ptVideoDevice->iPixelFormat = tDescFmt.pixelformat;
				break;
			#ifdef USE_FMT_YUYV
			}
			#endif
		}
		tDescFmt.index++; 
	}
	if(! ptVideoDevice->iPixelFormat) 
	{
		DEBUG_PRINTF("can not support the format of this device\n");
        goto fatal;   
	}
   
    /* set format in */
	//设置想要显示的分辨率 驱动程序会帮你选一个相近的
    memset(&tFmt, 0, sizeof(struct v4l2_format));
    tFmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    tFmt.fmt.pix.width       = iLcdWidth;
    tFmt.fmt.pix.height      = iLcdHeigt;
    tFmt.fmt.pix.pixelformat = ptVideoDevice->iPixelFormat; 
    tFmt.fmt.pix.field = V4L2_FIELD_ANY;
    ret = ioctl(ptVideoDevice->iFd, VIDIOC_S_FMT, &tFmt);
    if (ret < 0) 
	{
		DEBUG_PRINTF("Unable to set format.\n");
		goto fatal;
    }
	ptVideoDevice->iWidth  = tFmt.fmt.pix.width;
	ptVideoDevice->iHeight = tFmt.fmt.pix.height;
	
	/* request buffers */
	memset(&tRb, 0, sizeof(struct v4l2_requestbuffers));
	tRb.count  = NB_BUFFER;
	tRb.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	tRb.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(ptVideoDevice->iFd, VIDIOC_REQBUFS, &tRb);
	if (ret < 0) 
	{
		DEBUG_PRINTF("Unable to allocate buffers.\n");
		goto fatal;
	}
	//判断是哪种接口
	ptVideoDevice->iVideoBufCnt = tRb.count;
	if (tCap.capabilities & V4L2_CAP_STREAMING)
	{
		/* set framerate */
		/*
		struct v4l2_streamparm* setfps;  
		setfps=(struct v4l2_streamparm *) calloc(1, sizeof(struct v4l2_streamparm));
		memset(setfps, 0, sizeof(struct v4l2_streamparm));
		setfps->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		setfps->parm.capture.timeperframe.numerator=1;
		setfps->parm.capture.timeperframe.denominator=vd->fps;
		ret = ioctl(vd->fd, VIDIOC_S_PARM, setfps); 
		*/
		
		/* map the buffers */
		for (i = 0; i < NB_BUFFER; i++) 
		{
			memset(&tBuf, 0, sizeof(struct v4l2_buffer));
			tBuf.index  = i;
			tBuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			tBuf.memory = V4L2_MEMORY_MMAP;
			ret = ioctl(ptVideoDevice->iFd, VIDIOC_QUERYBUF, &tBuf);
			if (ret < 0) 
			{
				DEBUG_PRINTF("Unable to query buffer.\n");
				goto fatal;
			}
			ptVideoDevice->iVideoBufMaxLen = tBuf.length;
        	ptVideoDevice->pucVideBuf[i] = mmap(0 /* start anywhere */ ,
        			  tBuf.length, PROT_READ, MAP_SHARED, ptVideoDevice->iFd,
        			  tBuf.m.offset);
        	if (ptVideoDevice->pucVideBuf[i] == MAP_FAILED) 
            {
        	    DEBUG_PRINTF("Unable to map buffer\n");
        	    goto fatal;
        	}
		}
		/* Queue the buffers. */
		for (i = 0; i < NB_BUFFER; ++i) 
		{
			memset(&tBuf, 0, sizeof(struct v4l2_buffer));
			tBuf.index  = i;
			tBuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			tBuf.memory = V4L2_MEMORY_MMAP;
			ret = ioctl(ptVideoDevice->iFd, VIDIOC_QBUF, &tBuf);
			if (ret < 0) 
			{
				DEBUG_PRINTF("Unable to queue buffer.\n");
				goto fatal;
			}
		}
	
	}
	else if(tCap.capabilities & V4L2_CAP_READWRITE) 
	{
		tV4l2VideoOpr.GetFrame = V4l2GetFrameForReadWrite;
        tV4l2VideoOpr.PutFrame = V4l2PutFrameForReadWrite;
        
        /* read(fd, buf, size) */
        ptVideoDevice->iVideoBufCnt  = 1;
        /* 在这个程序所能支持的格式里, 一个象素最多只需要4字节 */
        ptVideoDevice->iVideoBufMaxLen = ptVideoDevice->iWidth * ptVideoDevice->iHeight * 4;
        ptVideoDevice->pucVideBuf[0] = malloc(ptVideoDevice->iVideoBufMaxLen);
	}
	//设置设备的操作函数
	ptVideoDevice->ptOPr =	&tV4l2VideoOpr;
    return 0;
  fatal:
    return -1;
}

static int V4l2ExitDevice(PT_VideoDevice ptVideoDevice)
{
	int i;
    for (i = 0; i < ptVideoDevice->iVideoBufCnt; i++)
    {
        if (ptVideoDevice->pucVideBuf[i])
        {
            munmap(ptVideoDevice->pucVideBuf[i], ptVideoDevice->iVideoBufMaxLen);
            ptVideoDevice->pucVideBuf[i] = NULL;
        }
    }
        
    close(ptVideoDevice->iFd);
	return 0;
}

static int V4l2GetFrame(PT_VideoDevice ptVideoDevice, PT_VideoBuf ptVideoBuf)
{
	struct pollfd tFds[1];
    int iRet;
    struct v4l2_buffer tV4l2Buf;
            
    /* poll */
    tFds[0].fd     = ptVideoDevice->iFd;
    tFds[0].events = POLLIN;

    iRet = poll(tFds, 1, -1);
    if (iRet <= 0)
    {
        DEBUG_PRINTF("poll error!\n");
        return -1;
    }
    
    /* VIDIOC_DQBUF */
    memset(&tV4l2Buf, 0, sizeof(struct v4l2_buffer));
    tV4l2Buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    tV4l2Buf.memory = V4L2_MEMORY_MMAP;
    iRet = ioctl(ptVideoDevice->iFd, VIDIOC_DQBUF, &tV4l2Buf);
    if (iRet < 0) 
    {
    	DEBUG_PRINTF("Unable to dequeue buffer.\n");
    	return -1;
    }
    ptVideoDevice->iVideoBufCurIndex = tV4l2Buf.index;

    ptVideoBuf->iPixelFormat        = ptVideoDevice->iPixelFormat;
    ptVideoBuf->tPixelDatas.iWidth  = ptVideoDevice->iWidth;
    ptVideoBuf->tPixelDatas.iHeight = ptVideoDevice->iHeight;
    ptVideoBuf->tPixelDatas.iBpp    = (ptVideoDevice->iPixelFormat == V4L2_PIX_FMT_YUYV) ? 16 : \
                                        (ptVideoDevice->iPixelFormat == V4L2_PIX_FMT_MJPEG) ? 0 :  \
                                        (ptVideoDevice->iPixelFormat == V4L2_PIX_FMT_RGB565) ? 16 :  \
                                        0;
    ptVideoBuf->tPixelDatas.iLineBytes    = ptVideoDevice->iWidth * ptVideoBuf->tPixelDatas.iBpp / 8;
    ptVideoBuf->tPixelDatas.iTotalBytes   = tV4l2Buf.bytesused;
    ptVideoBuf->tPixelDatas.aucPixelDatas = ptVideoDevice->pucVideBuf[tV4l2Buf.index];    
    return 0;
}

static int V4l2PutFrame(PT_VideoDevice ptVideoDevice, PT_VideoBuf ptVideoBuf)
{
	/* VIDIOC_QBUF */
    struct v4l2_buffer tV4l2Buf;
    int iError;
    
	memset(&tV4l2Buf, 0, sizeof(struct v4l2_buffer));
	tV4l2Buf.index  = ptVideoDevice->iVideoBufCurIndex;
	tV4l2Buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	tV4l2Buf.memory = V4L2_MEMORY_MMAP;
	iError = ioctl(ptVideoDevice->iFd, VIDIOC_QBUF, &tV4l2Buf);
	if (iError) 
    {
	    DEBUG_PRINTF("Unable to queue buffer.\n");
	    return -1;
	}
	return 0;
}

static int V4l2StartDevice(PT_VideoDevice ptVideoDevice)
{
	int iType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int iError;

    iError = ioctl(ptVideoDevice->iFd, VIDIOC_STREAMON, &iType);
    if (iError) 
    {
    	DEBUG_PRINTF("Unable to start capture.\n");
    	return -1;
    }
	return 0;
}

static int V4l2StopDevice(PT_VideoDevice ptVideoDevice)
{
	int iType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int iError;

    iError = ioctl(ptVideoDevice->iFd, VIDIOC_STREAMOFF, &iType);
    if (iError) 
    {
    	DEBUG_PRINTF("Unable to stop capture.\n");
    	return -1;
    }
	return 0;
}

static int V4l2GetFormat(PT_VideoDevice ptVideoDevice)
{
    return ptVideoDevice->iPixelFormat;
}

static T_VideoOpr tV4l2VideoOpr = {
	.name        = "v4l2",
    .InitDevice  = V4l2InitDevice,
    .ExitDevice  = V4l2ExitDevice,
	.GetFormat   = V4l2GetFormat,
    .GetFrame    = V4l2GetFrame,
    .PutFrame    = V4l2PutFrame,
    .StartDevice = V4l2StartDevice,
    .StopDevice  = V4l2StopDevice,
};

static int V4l2GetFrameForReadWrite(PT_VideoDevice ptVideoDevice, PT_VideoBuf ptVideoBuf)
{
	int iRet;

    iRet = read(ptVideoDevice->iFd, ptVideoDevice->pucVideBuf[0], ptVideoDevice->iVideoBufMaxLen);
    if (iRet <= 0)
    {
        return -1;
    }
    
    ptVideoBuf->iPixelFormat        = ptVideoDevice->iPixelFormat;
    ptVideoBuf->tPixelDatas.iWidth  = ptVideoDevice->iWidth;
    ptVideoBuf->tPixelDatas.iHeight = ptVideoDevice->iHeight;
    ptVideoBuf->tPixelDatas.iBpp    = (ptVideoDevice->iPixelFormat == V4L2_PIX_FMT_YUYV) ? 16 : \
                                        (ptVideoDevice->iPixelFormat == V4L2_PIX_FMT_MJPEG) ? 0 :  \
                                        (ptVideoDevice->iPixelFormat == V4L2_PIX_FMT_RGB565)? 16 : \
                                          0;
    ptVideoBuf->tPixelDatas.iLineBytes    = ptVideoDevice->iWidth * ptVideoBuf->tPixelDatas.iBpp / 8;
    ptVideoBuf->tPixelDatas.iTotalBytes   = iRet;
    ptVideoBuf->tPixelDatas.aucPixelDatas = ptVideoDevice->pucVideBuf[0];  
	return 0;
}

static int V4l2PutFrameForReadWrite(PT_VideoDevice ptVideoDevice, PT_VideoBuf ptVideoBuf)
{
	return 0;
}

int V4l2Init(void)
{
	return RegisterVideoOpr(&tV4l2VideoOpr);
}


