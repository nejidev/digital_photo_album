#include <encoding_manager.h>
#include <fonts_manager.h>
#include <string.h>

static int AsciiIsSupport(unsigned char *pucBufHead);
static int AsciiGetCodeFrmBuf(unsigned char *pucBufStart, unsigned char *pucBufEnd, unsigned int *pdwCode);

static T_EncodingOpr g_tAsciiEncodingOpr = {
	.name = "ascii",
	.iHeadLen = 0,
	.isSupport = AsciiIsSupport,
	.GetCodeFrmBuf = AsciiGetCodeFrmBuf,
};
static int AsciiIsSupport(unsigned char *pucBufHead)
{
	const char aStrUtf8[]    = {0xEF, 0xBB, 0xBF, 0};
	if(0 == strncmp(aStrUtf8, (const char *)pucBufHead, 3))
	{
		return 0;
	}
	return 1;
}

static int AsciiGetCodeFrmBuf(unsigned char *pucBufStart, unsigned char *pucBufEnd, unsigned int *pdwCode)
{
	//小于0x80 说明是 ascii 编码
	if(pucBufStart < pucBufEnd  && 0x80 > *pucBufStart)
	{
		*pdwCode = (unsigned int)*pucBufStart;
		return 1;
	}
	if((pucBufStart+1) < pucBufEnd && 0x80 < *pucBufStart && 0x80 < *(pucBufStart+1))
	{
		*pdwCode = *pucBufStart<<8 | *(pucBufStart+1);
		return 2;
	}
	
	//文件结束
	return 0;
}

int AsciiEncodingInit(void)
{
	int iError = 0;
	//初始化时 设置 FontOpr
	//调用的前题是已经初始化过 font asc gbk freetype
	//先加载的后加载
	iError |= AddFontOprForEncoding(&g_tAsciiEncodingOpr, GetFontOpr("freetype"));
	iError |= AddFontOprForEncoding(&g_tAsciiEncodingOpr, GetFontOpr("gbk"));
	iError |= AddFontOprForEncoding(&g_tAsciiEncodingOpr, GetFontOpr("ascii"));
	RegisterEncodingOpr(&g_tAsciiEncodingOpr);
	return iError;
}

