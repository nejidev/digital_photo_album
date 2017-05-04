#include <encoding_manager.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static PT_EncodingOpr g_ptEncodingOprHead;

int RegisterEncodingOpr(PT_EncodingOpr ptEncodingOpr)
{
	PT_EncodingOpr ptTempEncodingOpr;
	
	if(! g_ptEncodingOprHead)
	{
		g_ptEncodingOprHead = ptEncodingOpr;
		g_ptEncodingOprHead->ptNext = NULL;
	}
	else
	{
		ptTempEncodingOpr = g_ptEncodingOprHead;
		while(ptTempEncodingOpr->ptNext)
		{
			ptTempEncodingOpr = ptTempEncodingOpr->ptNext;
		}
		ptTempEncodingOpr->ptNext = ptEncodingOpr;
		ptEncodingOpr->ptNext = NULL;
	}
	return 0;
}

void ShowEncodingOpr(void)
{
	int i = 0;
	PT_EncodingOpr ptTempEncodingOpr = g_ptEncodingOprHead;
	while(ptTempEncodingOpr)
	{
		DEBUG_PRINTF("encoding:%d %s \n", i, ptTempEncodingOpr->name);
		ptTempEncodingOpr = ptTempEncodingOpr->ptNext;
	}
}

//为文件选择一个解码器
PT_EncodingOpr SelectEncodingOprForFile(unsigned char *pucFileBufHead)
{
	PT_EncodingOpr ptTempEncodingOpr = g_ptEncodingOprHead;
	while(ptTempEncodingOpr)
	{
		if(1 == ptTempEncodingOpr->isSupport(pucFileBufHead))
		{
			return ptTempEncodingOpr;
		}
		ptTempEncodingOpr = ptTempEncodingOpr->ptNext;
	}
	return NULL;
}

int AddFontOprForEncoding(PT_EncodingOpr ptEncodingOpr, PT_FontOpr ptFontOpr)
{
	PT_FontOpr ptFontOprCpy;
	if(! ptFontOpr || ! ptEncodingOpr)
	{
		return -1;
	}
	else
	{
		ptFontOprCpy = malloc(sizeof(T_FontOpr));
		if(! ptFontOprCpy)
		{
			return -1;
		}
		else
		{
			memcpy(ptFontOprCpy, ptFontOpr, sizeof(T_FontOpr));
			ptFontOprCpy->ptNext = ptEncodingOpr->ptFontOprSupportedHead;
			ptEncodingOpr->ptFontOprSupportedHead = ptFontOprCpy;
			return 0;
		}
	}
}

int DelFontOprFrmEncoding(PT_EncodingOpr ptEncodingOpr, PT_FontOpr ptFontOpr)
{
	PT_FontOpr ptTmp;
	PT_FontOpr ptPre;
		
	if (! ptEncodingOpr || ! ptFontOpr)
	{
		return -1;
	}
	else
	{
		ptTmp = ptEncodingOpr->ptFontOprSupportedHead;
		if (0 == strcmp(ptTmp->name, ptFontOpr->name))
		{
			/* 删除头节点 */
			ptEncodingOpr->ptFontOprSupportedHead = ptTmp->ptNext;
			free(ptTmp);
			return 0;
		}
		ptPre = ptEncodingOpr->ptFontOprSupportedHead;
		ptTmp = ptPre->ptNext;
		while (ptTmp)
		{
			if (0 == strcmp(ptTmp->name, ptFontOpr->name))
			{
				/* 删除头节点 */
				ptPre->ptNext = ptTmp->ptNext;
				free(ptTmp);
				return 0;
			}
			else
			{
				ptPre = ptTmp;
				ptTmp = ptTmp->ptNext;
			}
		}
	}
	return -1;
}

int EncodingInit(void)
{
	int iError = 0;
	iError |= AsciiEncodingInit();
	iError |= Utf8EncodingInit();
	return iError;
}
