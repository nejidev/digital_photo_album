#ifndef __PIC_OPERATION_H
#define __PIC_OPERATION_H

typedef struct PixelDatas{
	int iWidth;
	int iHeight;
	int iBpp;
	int iLineBytes;
	int iTotalBytes;
	unsigned char *aucPixelDatas;
}T_PixelDatas, *PT_PixelDatas;

typedef struct PicFileParser{
	char *name;
	int (*isSupport)(unsigned char *aFileHead, int iMemlen);
	int (*GetPixelDatas)(unsigned char *aFileHead, PT_PixelDatas ptPixelDatas);
	int (*FreePixelDatas)(PT_PixelDatas ptPixelDatas);
	int (*initPicFileParser)(void);
	struct PicFileParser *ptNext;
}T_PicFileParser, *PT_PicFileParser;

int RegisterParse(PT_PicFileParser ptPicFileParser);
PT_PicFileParser MatchParser(unsigned char *pcPicHead, int iMemlen);
PT_PicFileParser Parser(char *pcName);
int InitParse(void);
int BmpInitParse(void);
int JPEGInitParse(void);

#endif

