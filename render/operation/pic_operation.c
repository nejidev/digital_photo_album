#include <pic_operation.h>
#include <string.h>
#include <stdio.h>

static PT_PicFileParser g_ptPicFileParserHead;

int RegisterParse(PT_PicFileParser ptPicFileParser)
{
	PT_PicFileParser ptTmp;
	if(! g_ptPicFileParserHead)
	{
		g_ptPicFileParserHead = ptPicFileParser;
	}
	else
	{
		ptTmp = g_ptPicFileParserHead;
		while(ptTmp->ptNext)
		{
			ptTmp = ptTmp->ptNext;
		}
		ptTmp->ptNext = ptPicFileParser;
	}
	ptPicFileParser->ptNext = NULL;
	
	//如果有设置初始化函数就执行它
	if(ptPicFileParser->initPicFileParser)
	{
		return ptPicFileParser->initPicFileParser();
	}
	return 0;
}

PT_PicFileParser MatchParser(unsigned char *pcPicHead, int iMemlen)
{
	PT_PicFileParser ptTmp = g_ptPicFileParserHead;
	while(ptTmp)
	{
		if(ptTmp->isSupport(pcPicHead, iMemlen))
		{
			return ptTmp;
		}
		ptTmp = ptTmp->ptNext;
	}
	return NULL;
}

PT_PicFileParser Parser(char *pcName)
{
	PT_PicFileParser ptTmp = g_ptPicFileParserHead;
	
	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			return ptTmp;
		}
		ptTmp = ptTmp->ptNext;
	}
	return NULL;
}

int InitParse(void)
{
	int iError = 0;
	iError |= BmpInitParse();
	iError |= JPEGInitParse();
	return iError;
}

