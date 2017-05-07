#include <fonts_manager.h>
#include <stdio.h>
#include <string.h>

static PT_FontOpr g_ptFontOprHead;

int RegisterFontOpr(PT_FontOpr ptFontOpr)
{
	PT_FontOpr pt_tempFontOpr;
	//如果为空就赋值
	if(! g_ptFontOprHead)
	{
		g_ptFontOprHead = ptFontOpr;
		ptFontOpr->ptNext = NULL;
	}
	else
	{
		pt_tempFontOpr = g_ptFontOprHead;
		//查找到最后一个
		while(pt_tempFontOpr->ptNext)
		{
			pt_tempFontOpr = pt_tempFontOpr->ptNext;
		}
		pt_tempFontOpr->ptNext = ptFontOpr;
		ptFontOpr->ptNext = NULL;
	}
	return 0;
}

void ShowFontOpr(void)
{
	int i = 0;
	PT_FontOpr pt_tempFontOpr = g_ptFontOprHead;
	while(pt_tempFontOpr)
	{
		DEBUG_PRINTF("fonts:%d %s \n", i++, pt_tempFontOpr->name);
		pt_tempFontOpr = pt_tempFontOpr->ptNext;
	}
}

int InitFonts(void)
{
	int iError = 0;
	iError |= ASCIIInit();
	iError |= GBKInit();
	iError |= FreeTypeInit();	
	return iError;
}

PT_FontOpr GetFontOpr(char *pcName)
{
	PT_FontOpr pt_tempFontOpr = g_ptFontOprHead;
	while(pt_tempFontOpr)
	{
		
		if(0 == strcmp(pcName, pt_tempFontOpr->name))
		{
			//获取到之前初始化 不可以在这里初始化 因为还要传参数
			//pt_tempFontOpr->FontInit();
			return pt_tempFontOpr;
		}
		pt_tempFontOpr = pt_tempFontOpr->ptNext;
	}
	return NULL;
}
