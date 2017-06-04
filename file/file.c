#include <conf.h>
#include <file.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <disp_manager.h>
#include <render.h>

#include <dirent.h>

static int isDir(char *strFilePath, char *strFileName)
{
    char strTmp[256];
    struct stat tStat;

    snprintf(strTmp, 256, "%s/%s", strFilePath, strFileName);
    strTmp[255] = '\0';

    if ((stat(strTmp, &tStat) == 0) && S_ISDIR(tStat.st_mode))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static int isFile(char *strFilePath, char *strFileName)
{
    char strTmp[256];
    struct stat tStat;

    snprintf(strTmp, 256, "%s/%s", strFilePath, strFileName);
    strTmp[255] = '\0';

    if ((stat(strTmp, &tStat) == 0) && S_ISREG(tStat.st_mode))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int MapFile(PT_FileMap ptFileMap)
{
	struct stat stat;
	FILE *fp;
	int  fd;
	
	fp = fopen(ptFileMap->strFileName, "r+");
	if(NULL == fp)
	{
		DEBUG_PRINTF("cat't open file %s error\n", ptFileMap->strFileName);
		return -1;
	}
	fd = fileno(fp);
	if(-1 == fstat(fd, &stat))
	{
		DEBUG_PRINTF("cat't get file stat %s error\n", ptFileMap->strFileName);
		return -1;
	}
 	ptFileMap->pucFileMapMem = (unsigned char *)mmap(NULL, stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(MAP_FAILED == ptFileMap->pucFileMapMem)
	{
		DEBUG_PRINTF("cat't mmap file %s error\n", ptFileMap->strFileName);
		return -1;
	}
	ptFileMap->iFileSize     = stat.st_size;
	ptFileMap->tFp           = fp;

	return 0;
}

void UnMapFile(PT_FileMap ptFileMap)
{
	munmap(ptFileMap->pucFileMapMem, ptFileMap->iFileSize);
	fclose(ptFileMap->tFp);
}

int GetDirContents(char *strDirName, PT_DirFiles **pptDirFiles, int *piNumber)
{
	PT_DirFiles *ptDirFiles;
	struct dirent **aptNameList;
	int iNum;
	int i;
	iNum = scandir(strDirName, &aptNameList, 0, alphasort);
	if(-1 == iNum)
	{
		DEBUG_PRINTF("scandir err %s \n", strDirName);
	 	return -1;
	}
	//分配内存 
	ptDirFiles = malloc(sizeof(PT_DirFiles) * iNum);
	if(! ptDirFiles)
	{
		DEBUG_PRINTF("GetDirContents %s malloc error \n", strDirName);
		return -1;
	}
	//双指针的 指针等于新分配的
	*pptDirFiles = ptDirFiles;
	
	//填充 ptDirFiles
	for(i=0; i<iNum; i++)
	{
		if(isDir(strDirName, aptNameList[i]->d_name))
		{
			ptDirFiles[i] = malloc(sizeof(T_DirFiles));
			if(! ptDirFiles[i])
			{	
				DEBUG_PRINTF("malloc error \n");
				return -1;
			}
			strncpy(ptDirFiles[i]->strName, aptNameList[i]->d_name, 256);
			ptDirFiles[i]->strName[255] = '\n';
			ptDirFiles[i]->eFileType = FILETYPE_DIR;
			(*piNumber)++;
		}
		if(isFile(strDirName, aptNameList[i]->d_name))
		{
			ptDirFiles[i] = malloc(sizeof(T_DirFiles));
			if(! ptDirFiles[i])
			{	
				DEBUG_PRINTF("malloc error \n");
				return -1;
			}
			strncpy(ptDirFiles[i]->strName, aptNameList[i]->d_name, 256);
			ptDirFiles[i]->strName[255] = '\n';
			ptDirFiles[i]->eFileType = FILETYPE_FILE;
			(*piNumber)++;
		}
	}
	
	/* 释放scandir函数分配的内存 */
	for(i=0; i<iNum; i++)
	{
		if(aptNameList[i])
		{
			free(aptNameList[i]);
		}
	}
	//释放内存
	free(aptNameList);
	return 0;
}

int GetFileICON(PT_PixelDatas ptIconPixelDatas, char *name)
{
	int iXres, iYres, iBpp;
	
	//读取Icon图片缩放到合适大小
 	T_PixelDatas tOriginIconPixelDatas;
	
	GetDispResolution(&iXres, &iYres, &iBpp);
	tOriginIconPixelDatas.iBpp = iBpp;

	//得到 BMP 图片数据	
	if(GetPixelDatasForIcon(name, &tOriginIconPixelDatas))
	{
		DEBUG_PRINTF("GetPixelDatasForIcon error \n");
		return -1;
	}
	
	//执行缩放
	PicZoom(&tOriginIconPixelDatas, ptIconPixelDatas);
	
	//释放 BMP 图片数据内存
	free(tOriginIconPixelDatas.aucPixelDatas);
	
	return 0;
}

int FileExist(char *file)
{
	struct stat tStat;
	if ((0 == stat(file, &tStat)) && 0 < tStat.st_size)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int GetFileIconName(PT_DirFiles ptDirFiles, char *pcIconName)
{
	char *pos;
	char ext[256];
	if(! pcIconName)
	{
		DEBUG_PRINTF("malloc error \n");
		return -1;
	}
	//文件夹
	if(FILETYPE_DIR == ptDirFiles->eFileType)
	{
		strcpy(ext, "dir");
	}
	else
	{
		//取文件后缀后
		//算法: 先查找最后一个 . 的位置指针
		pos = strrchr(ptDirFiles->strName, '.');
		//未查找到后缀名 使用 text
		if(! pos)
		{
			strcpy(ext, "text");
		}
		else
		{
			strcpy(ext, pos + 1);
		}
		//检查对应的 icon 图标文件是否存在 不存在就用 text
	}
	snprintf(pcIconName, 256, "%s%s.bmp", ICON_DIR, ext);
	//如果图标文件不存在 就使用默认的 text.bmp
	if(! FileExist(pcIconName))
	{
		snprintf(pcIconName, 256, "%stext.bmp", ICON_DIR);
	}
	return 0;
}
