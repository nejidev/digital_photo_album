#include <fonts_manager.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
	   
static int GbkFontInit(char *pcFontFile, unsigned int dwFontSize);
static int GbkGetFontBitMap(unsigned int dwCode, PT_FontBitMap ptFontBitMap);

static T_FontOpr g_tGbkFontOpr = {
	.name          = "gbk",
	.FontInit      = GbkFontInit,
	.GetFontBitMap = GbkGetFontBitMap,
};

static int gbk_fd;
static unsigned char *gbk_buf;
static unsigned char *gbk_buf_end;
static struct stat gbk_stat;

static int GbkFontInit(char *pcFontFile, unsigned int dwFontSize)
{
	if(16 != dwFontSize)
	{
		DEBUG_PRINTF("cat't gbk font init font size error : %d \n", dwFontSize);
		return -1;
	}
	gbk_fd = open(pcFontFile, O_RDONLY);
	if(0 > gbk_fd)
	{
		DEBUG_PRINTF("cat't open %s \n", pcFontFile);
		return -1;
	}
	if(fstat(gbk_fd, &gbk_stat))
	{
		DEBUG_PRINTF("cat't open %s \n", pcFontFile);
		return -1;
	}
	gbk_buf = mmap(NULL, gbk_stat.st_size, PROT_READ, MAP_SHARED, gbk_fd, 0);
	if(MAP_FAILED == gbk_buf)
	{
		return -1;
		DEBUG_PRINTF("cat't mmap %s \n", pcFontFile);
	}
	gbk_buf_end = gbk_buf + gbk_stat.st_size;
	return 0;
}

static int GbkGetFontBitMap(unsigned int dwCode, PT_FontBitMap ptFontBitMap)
{
	int iPenX = ptFontBitMap->iCurOriginX;
	int iPenY = ptFontBitMap->iCurOriginY;

	int area, code;
	area = ((dwCode>>8) & 0xff) - 0xa1;
	code = ((dwCode>>0) & 0xff) - 0xa1;

	//检查 是否是 GBK
	if(0 > area || 0 > code)
	{
		return -1;
	}
	
	//设置画板区域
	ptFontBitMap->iXLeft = iPenX;
	ptFontBitMap->iYTop  = iPenY - 16;
	ptFontBitMap->iXMax  = iPenX + 16;
	ptFontBitMap->iYMax  = iPenY;
	ptFontBitMap->iBpp   = 1;
	ptFontBitMap->iPitch = 1;

	//设置字符buff
	ptFontBitMap->pucBuffer = gbk_buf + (area*94+code)*32;
	if(gbk_buf_end < ptFontBitMap->pucBuffer)
	{
		return -1;
	}
	ptFontBitMap->iNextOriginX = iPenX + 16;
	ptFontBitMap->iNextOriginY = iPenY;
	return 0;
}

int GBKInit(void)
{
	return RegisterFontOpr(&g_tGbkFontOpr);
}