#include <conf.h>
#include <file.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

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

