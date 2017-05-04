#include <fonts_manager.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
	   
static int FreetypeFontInit(char *pcFontFile, unsigned int dwFontSize);
static int FreetypeGetFontBitMap(unsigned int dwCode, PT_FontBitMap ptFontBitMap);

static T_FontOpr g_tFreetypeFontOpr = {
	.name          = "freetype",
	.FontInit      = FreetypeFontInit,
	.GetFontBitMap = FreetypeGetFontBitMap,
};

static FT_Library   g_tLibrary;
static FT_Face      g_tFace;
static FT_GlyphSlot g_tSlot;

static int FreetypeFontInit(char *pcFontFile, unsigned int dwFontSize)
{
	int iError = 0;
	iError = FT_Init_FreeType(&g_tLibrary);
	if (iError)
	{
		DEBUG_PRINTF("FT_Init_FreeType failed\n");
		return -1;
	}
	
	//加载字体文件 0 face_index
	iError = FT_New_Face(g_tLibrary, pcFontFile, 0, &g_tFace);
	if (iError)
	{
		DEBUG_PRINTF("FT_Init_FreeType failed\n");
		return -1;
	}
	
	//设置 大小
	iError = FT_Set_Pixel_Sizes(g_tFace, dwFontSize, dwFontSize);
	if (iError)
	{
		DEBUG_PRINTF("FT_Set_Pixel_Sizes failed\n");
		return -1;
	}
	g_tSlot  = g_tFace->glyph;
	return 0;
}

static int FreetypeGetFontBitMap(unsigned int dwCode, PT_FontBitMap ptFontBitMap)
{
	int iError;
	int iPenX = ptFontBitMap->iCurOriginX;
	int iPenY = ptFontBitMap->iCurOriginY;

	iError = FT_Load_Char(g_tFace, dwCode, FT_LOAD_RENDER | FT_LOAD_MONOCHROME);
	if (iError)
	{
		DEBUG_PRINTF("FT_Load_Char error for code : 0x%x\n", dwCode);
		return -1;
	}

	//渲染
	//FT_Set_Transform(face, 0, &pen);
	
	//设置画板区域
	ptFontBitMap->iXLeft    = iPenX + g_tSlot->bitmap_left;
	ptFontBitMap->iYTop     = iPenY - g_tSlot->bitmap_top;
	//ptFontBitMap->iXLeft = iPenX;
	//ptFontBitMap->iYTop  = iPenY - g_tSlot->bitmap.rows;
	ptFontBitMap->iXMax  = ptFontBitMap->iXLeft + g_tSlot->bitmap.width;
	ptFontBitMap->iYMax  = ptFontBitMap->iYTop + g_tSlot->bitmap.rows;
	ptFontBitMap->iBpp      = 1;
	ptFontBitMap->iPitch    = g_tSlot->bitmap.pitch;
	ptFontBitMap->pucBuffer = g_tSlot->bitmap.buffer;

	//设置字符buff
	ptFontBitMap->iNextOriginX = iPenX + g_tSlot->advance.x / 64;
	ptFontBitMap->iNextOriginY = iPenY;
	return 0;
}

int FreeTypeInit(void)
{
	return RegisterFontOpr(&g_tFreetypeFontOpr);
}
