#ifndef __VIDEO_OPERATION_H
#define __VIDEO_OPERATION_H
#include <conf.h>
#include <stdio.h>
#include <convert_manager.h>
#include <video_manager.h>

int VideoPlayStart(void);
int VideoPlayStop(void);
int VideoSaveJpg(char *dir);

#endif

