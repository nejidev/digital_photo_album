#include <encoding_manager.h>
#include <fonts_manager.h>
#include <string.h>

static int Utf8IsSupport(unsigned char *pucBufHead);
static int Utf8GetCodeFrmBuf(unsigned char *pucBufStart, unsigned char *pucBufEnd, unsigned int *pdwCode);

static T_EncodingOpr g_tUtf8EncodingOpr = {
	.name = "utf8",
	.iHeadLen = 3,
	.isSupport = Utf8IsSupport,
	.GetCodeFrmBuf = Utf8GetCodeFrmBuf,
};
static int Utf8IsSupport(unsigned char *pucBufHead)
{
	const char aStrUtf8[]    = {0xEF, 0xBB, 0xBF, 0};
	if(0 == strncmp(aStrUtf8, (const char*)pucBufHead, 3))
	{
		return 1;
	}
	return 0;
}

/* 获得前导为1的位的个数
 * 比如二进制数 11001111 的前导1有2位
 *              11100001 的前导1有3位
 */
static int GetPreOneBits(unsigned char ucchar)
{
	int len = 0;
	int i;
	for(i=7; i>=0; i--)
	{
		if(ucchar & (1<<i))
		{
			len++;
		}
		else
		{
			break;
		}
	}
	return len;
}

static int Utf8GetCodeFrmBuf(unsigned char *pucBufStart, unsigned char *pucBufEnd, unsigned int *pdwCode)
{
	//utf8 编码规则
	/*
    对于UTF-8编码中的任意字节B，如果B的第一位为0，则B为ASCII码，并且B独立的表示一个字符;
    如果B的第一位为1，第二位为0，则B为一个非ASCII字符（该字符由多个字节表示）中的一个字节，并且不为字符的第一个字节编码;
    如果B的前两位为1，第三位为0，则B为一个非ASCII字符（该字符由多个字节表示）中的第一个字节，并且该字符由两个字节表示;
    如果B的前三位为1，第四位为0，则B为一个非ASCII字符（该字符由多个字节表示）中的第一个字节，并且该字符由三个字节表示;
    如果B的前四位为1，第五位为0，则B为一个非ASCII字符（该字符由多个字节表示）中的第一个字节，并且该字符由四个字节表示;

    因此，对UTF-8编码中的任意字节，根据第一位，可判断是否为ASCII字符;
    根据前二位，可判断该字节是否为一个字符编码的第一个字节; 
    根据前四位（如果前两位均为1），可确定该字节为字符编码的第一个字节，并且可判断对应的字符由几个字节表示;
    根据前五位（如果前四位为1），可判断编码是否有错误或数据传输过程中是否有错误。
	*/
	int preBit = GetPreOneBits(*pucBufStart);
	if (pucBufStart >= pucBufEnd)
	{
		/* 文件结束 */
		return 0;
	}
	//ascii 编码
	if((0 == preBit) && pucBufStart < pucBufEnd)
	{
		*pdwCode = *pucBufStart;
		pucBufStart++;
		return 1;
	}
	if((pucBufStart + preBit) < pucBufEnd)
	{
		//“严”的UTF-8编码是“11100100 10111000 10100101”，转换成十六进制就是E4B8A5。
		if(2 == preBit)
		{
			*pdwCode = (*pucBufStart & 0x1f)<<6 | (*(pucBufStart+1) & 0x3f);
		}
		if(3 == preBit)
		{
			*pdwCode = (*pucBufStart & 0xf)<<12 | (*(pucBufStart+1) & 0x3f)<<6 | (*(pucBufStart+2) & 0x3f);
		}
		pucBufStart += preBit;
		return preBit;
	}
	//文件结束
	return 0;
}

int Utf8EncodingInit(void)
{
	int iError = 0;
	//初始化时 设置 FontOpr
	//调用的前题是已经初始化过 font asc gbk freetype
	iError |= AddFontOprForEncoding(&g_tUtf8EncodingOpr, GetFontOpr("freetype"));
	iError |= AddFontOprForEncoding(&g_tUtf8EncodingOpr, GetFontOpr("ascii"));
	RegisterEncodingOpr(&g_tUtf8EncodingOpr);
	return iError;
}
