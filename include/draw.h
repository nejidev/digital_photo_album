#ifndef __DRAW_H
#define __DRAW_H
typedef struct PageDesc{
	int iPage;
	unsigned char *pucLcdFirstPosAtFile;
	unsigned char *pucLcdNextPageFirstPosAtFile;
	struct PageDesc *ptPrePage;
	struct PageDesc *ptNextPage;
} T_PageDesc, *PT_PageDesc;

int OpenTextFile(char *pcFileName);
int SetFontsDetail(char *pcHZKFile, char *pcFileFreetype, unsigned dwFontSize);
int SelectAndInitDisplay(char *pcName);
int ShowFirstPage(void);
int ShowNextPage(void);
int ShowPrePage(void);
#endif

