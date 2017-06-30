
#ifndef _CONVERT_MANAGER_H
#define _CONVERT_MANAGER_H

#include <conf.h>
#include <video_manager.h>

typedef struct VideoConvert {
	char *name;
    int (*isSupport)(int iPixelFormatIn, int iPixelFormatOut);
    int (*Convert)(PT_VideoBuf ptVideoBufIn, PT_VideoBuf ptVideoBufOut);
    int (*ConvertExit)(PT_VideoBuf ptVideoBufOut);
	struct VideoConvert *ptNext;
}T_VideoConvert, *PT_VideoConvert;

int RegisterVideoConvert(PT_VideoConvert ptVideoConvert);
PT_VideoConvert GetVideoConvertForFormats(int iPixelFormatIn, int iPixelFormatOut);
int InitVideoConvert(void);
int InitMjpeg2rgb(void);
int InitYuv2rgb(void);
int InitRgb2rgb(void);

#endif /* _CONVERT_MANAGER_H */

