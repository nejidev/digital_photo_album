#ifndef __CONFIG_H
#define __CONFIG_H
#include <stdio.h>
#define FB_DEVICE "/dev/fb0"         //FB λ��
#define VIDEO_DEV "/dev/video0"      //����ͷλ��
#define COLOR_BACKGROUND 0xe7dbb5 //���Ƶ�ֽ
#define COLOR_FOREGROUND 0x514438 //������ɫ

#define ICON_DIR "./icon/"

#define DIR_FILE_ICON_WIDTH    40
#define DIR_FILE_ICON_HEIGHT   DIR_FILE_ICON_WIDTH
#define DIR_FILE_NAME_HEIGHT   20
#define DIR_FILE_NAME_WIDTH   (DIR_FILE_ICON_HEIGHT + DIR_FILE_NAME_HEIGHT)
#define DIR_FILE_ALL_WIDTH    DIR_FILE_NAME_WIDTH
#define DIR_FILE_ALL_HEIGHT   DIR_FILE_ALL_WIDTH

//#define DEBUG_PRINTF(...)
#define DEBUG_PRINTF printf

#endif

