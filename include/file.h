#ifndef __FILE_H
#define __FILE_H
#include <stdio.h>

/* �ļ���� */
typedef enum {
	FILETYPE_DIR = 0,  /* Ŀ¼ */
	FILETYPE_FILE,     /* �ļ� */
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

#endif
