#ifndef __FILE_H
#define __FILE_H
#include <stdio.h>
#include <pic_operation.h>

/* 文件类别 */
typedef enum {
	FILETYPE_DIR = 0,  /* 目录 */
	FILETYPE_FILE,     /* 文件 */
}E_FileType;

typedef struct FileMap{
	char strFileName[256];
	FILE *tFp;
	int iFileSize;
	unsigned char *pucFileMapMem;
}T_FileMap, *PT_FileMap;

typedef struct DirFiles{
	char strName[256];
	E_FileType eFileType;
}T_DirFiles, *PT_DirFiles;

int MapFile(PT_FileMap ptFileMap);
void UnMapFile(PT_FileMap ptFileMap);
int GetDirContents(char *strDirName, PT_DirFiles **pptDirFiles, int *piNumber);
int GetFileICON(PT_PixelDatas ptIconPixelDatas, char *name);
int GetFileIconName(PT_DirFiles ptDirFiles, char *pcIconName);
int FileExist(char *file);

#endif

